#include "Engine.h"
#include <algorithm>
#include <atomic>
#include <new>
#include <memory>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace field;
namespace {
std::atomic<bool> watchAllocations{false};
std::atomic<size_t> watchedAllocations{0};
}
void* operator new(std::size_t n){if(watchAllocations.load())++watchedAllocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
float energy(const std::vector<float>& v){double total=0;for(float x:v)total+=x*x;return static_cast<float>(total/std::max<size_t>(1,v.size()));}
void render(Engine& e,std::vector<float>& l,std::vector<float>& r,int frames){l.resize(frames);r.resize(frames);const auto allocations=watchedAllocations.load();watchAllocations=true;
    e.process(l.data(),r.data(),nullptr,nullptr,frames);watchAllocations=false;
    check(watchedAllocations.load()==allocations,"audio render performs no heap allocation");}
}
int main(){
    std::vector<float> l,r;
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(48000,512);Parameters p;p.engine=1;p.macro[1]=0;p.attack=.001f;p.decay=.01f;p.sustain=1;p.release=.025f;p.volume=1;e.setParameters(p);e.noteOn(69,1);render(e,l,r,48000);
        check(energy(l)>.001f,"FM sine carrier produces audio");
        int crossings=0;for(size_t i=4801;i<l.size();++i)if(l[i-1]<=0&&l[i]>0)++crossings;
        check(std::abs(crossings/.9-440)<2,"MIDI A4 is tuned to 440Hz");
        e.noteOff(69);render(e,l,r,4800);double tail=0;for(size_t i=3000;i<l.size();++i)tail+=std::abs(l[i]);check(tail<.00001,"note-off envelope stops voice");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,128);Parameters p;p.monitorInput=true;p.volume=1;e.setParameters(p);
        e.setRecording(true);l.assign(800,.2f);r.assign(800,-.1f);e.process(l.data(),r.data(),l.data(),r.data(),800);e.setRecording(false);e.setPlaying(false);
        check(e.tracks()[0].left.size()==800,"recording writes exact frame count");
        check(std::abs(e.tracks()[0].left[20]-.2f)<1e-6,"recording preserves left input");
        check(std::abs(e.tracks()[0].right[20]+.1f)<1e-6,"recording preserves stereo input");
        check(std::abs(e.positionSeconds()-.1)<1e-9,"transport advances by audio time");
        e.rewind();e.setRecording(true);l.assign(800,.1f);r.assign(800,.1f);e.process(l.data(),r.data(),l.data(),r.data(),800);e.setRecording(false);e.setPlaying(false);
        check(std::abs(e.tracks()[0].left[100]-.3f)<1e-6,"overdub adds to existing audio");
        e.undo();check(std::abs(e.tracks()[0].left[100]-.2f)<1e-6,"undo restores previous overdub");e.redo();check(std::abs(e.tracks()[0].left[100]-.3f)<1e-6,"redo restores overdub");
        e.lift(.025,.05);check(e.tracks()[0].left[250]==0,"lift silences source without ripple");check(e.tracks()[0].left.size()==800,"lift preserves tape timeline");
        e.selectTrack(1);e.seek(.05);e.drop();check(e.tracks()[1].left.size()==600,"drop places copied span at playhead");check(std::abs(e.tracks()[1].left[450]-.3f)<1e-6,"clipboard crosses tracks");
        e.seek(.06);e.split();check(e.tracks()[1].cuts.size()>=3,"split records splice boundary");
        e.prepare(16000,128);check(std::abs(e.recordedSeconds()-.1)<1e-9,"sample-rate change preserves tape duration");check(std::abs(e.tracks()[0].left[100]-.3f)<1e-6,"sample-rate change preserves tape samples");
        p.loop=true;e.setParameters(p);e.setLoop(.02,.04);e.seek(.03);e.setPlaying(true);render(e,l,r,800);check(e.positionSeconds()>=.02&&e.positionSeconds()<=.04+1.0/16000,"loop wraps to selected span");
        p.tapeSpeed=-1;e.setParameters(p);e.seek(.03);render(e,l,r,800);check(e.positionSeconds()>=.02&&e.positionSeconds()<=.04+1.0/16000,"reverse playback respects loop");
        e.setPlaying(false);auto mix=e.exportMix();check(mix.first.size()==1600&&mix.second.size()==1600,"stereo mix export has project duration");
        e.setTrackMute(0,true);e.setTrackMute(1,true);mix=e.exportMix();check(energy(mix.first)==0,"mute is respected in export");
        e.setTrackMute(0,false);e.setTrackSolo(1,true);mix=e.exportMix();check(energy(mix.first)==0,"solo excludes other tracks in export");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,128);Parameters p;p.engine=11;p.attack=.001f;p.sustain=1;p.macro[0]=0;p.macro[1]=1;p.macro[2]=1;e.setParameters(p);
        e.loadSample(std::vector<float>(8000,.4f),std::vector<float>(8000,-.2f),8000);e.noteOn(60,1);render(e,l,r,1600);
        check(l[100]>.01f&&r[100]<-.005f,"sampler preserves stereo polarity");
        e.allNotesOff();render(e,l,r,4000);p.drumMode=true;e.setParameters(p);e.noteOn(53,1);render(e,l,r,1600);double late=0;for(int i=700;i<1600;++i)late+=std::abs(l[i]);check(late<1e-5,"drum slice stops at slice endpoint");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);Parameters p;p.release=.02f;
        for(int engine=0;engine<15;++engine){p.engine=engine;e.setParameters(p);e.noteOn(60,.8f);render(e,l,r,1600);for(float x:l)check(std::isfinite(x)&&std::abs(x)<=1,"all engines remain finite and bounded");if(engine<11 || engine==14)check(energy(l)>1e-6,"synthesis engine produces signal");e.allNotesOff();render(e,l,r,800);}
        for(int fx=0;fx<11;++fx){p.engine=0;p.effect=fx;p.effectMix=.7f;e.setParameters(p);e.noteOn(60,.8f);render(e,l,r,4000);for(float x:l)check(std::isfinite(x)&&std::abs(x)<=1,"effects stay finite and bounded");e.allNotesOff();render(e,l,r,800);}
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);Parameters p;p.sequenceOn=true;p.sequencer=1;p.tempo=120;p.attack=.001f;p.release=.01f;e.setParameters(p);e.noteOn(60,1);e.noteOn(64,.8f);render(e,l,r,8000);check(energy(l)>.00001,"arpeggiator drives synth from held notes");check(e.currentSequenceStep()==8,"sequencer advances at sixteenth-note tempo");e.noteOff(60);e.noteOff(64);render(e,l,r,4000);render(e,l,r,1000);check(energy(l)<1e-9,"releasing arpeggiator keys stops sequence");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,128);e.tracks()[0].left.assign(800,.25f);e.tracks()[0].right.assign(800,-.15f);e.restoredTape();
        Parameters p;
        for(int style=0;style<4;++style){p.tapeStyle=style;e.setParameters(p);auto offline=e.exportMix();e.rewind();e.setPlaying(true);render(e,l,r,400);check(std::abs(offline.first[200]-l[200])<1e-6f&&std::abs(offline.second[200]-r[200])<1e-6f,"offline export matches live tape coloration");e.setPlaying(false);}
        std::vector<float> source(8000*30,.1f);e.loadSample(source,source,8000);check(e.sampleLeft().size()==source.size(),"sample storage preserves more than twenty seconds");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);std::vector<float> ramp(8000),inverse(8000);for(int i=0;i<8000;++i){ramp[i]=i/8000.f;inverse[i]=-ramp[i];}e.loadSample(ramp,inverse,8000);
        Parameters p;p.engine=11;p.attack=.001f;p.decay=.005f;p.sustain=1;p.release=.01f;p.sampleStart=.25f;p.sampleEnd=.5f;e.setParameters(p);e.noteOn(60,1);render(e,l,r,1000);check(l[500]>.035f,"melodic sampler starts inside trimmed region");render(e,l,r,2000);check(l.back()==0,"melodic sampler ends at trim boundary");
        p.sampleReverse=true;e.setParameters(p);e.noteOn(60,1);render(e,l,r,1000);check(l[100]>l[500],"reverse sampler traverses source backwards");e.allNotesOff();render(e,l,r,500);
        p.sampleReverse=false;p.sampleLoop=true;p.sampleLoopIn=.3f;p.sampleLoopOut=.35f;e.setParameters(p);e.noteOn(60,1);render(e,l,r,8000);check(energy(l)>.0001f&&std::abs(l.back())>.01f,"melodic sampler sustain loop remains active");e.noteOff(60);render(e,l,r,1000);check(l.back()==0,"gated loop releases after key-off");
        p.sampleLoop=false;p.sampleOneShot=true;e.setParameters(p);e.noteOn(60,1);render(e,l,r,100);e.noteOff(60);render(e,l,r,800);check(energy(l)>.0001f,"one-shot sample ignores key-off");render(e,l,r,2000);
        p.drumMode=true;e.setParameters(p);DrumPad pad;pad.start=.5f;pad.end=.75f;pad.tune=12;pad.pan=1;pad.mode=0;e.setDrumPad(0,pad);e.noteOn(53,1);render(e,l,r,1400);check(energy(l)==0&&energy(r)>.0001f,"drum pad pan preserves hard-right stereo routing");check(r.back()==0,"pad semitone tuning changes slice duration");check(e.selectedPad()==0,"first physical F3 key selects pad one");
        pad.pan=0;pad.tune=0;pad.mode=2;e.setDrumPad(0,pad);e.noteOn(53,1);render(e,l,r,6000);check(std::abs(l.back())>.01f,"drum pad loop can sustain beyond its slice");e.noteOff(53);render(e,l,r,1000);check(l.back()==0,"drum pad loop obeys gate release");
        pad.mode=0;pad.level=0;e.setDrumPad(0,pad);e.noteOn(53,1);render(e,l,r,1000);check(energy(l)==0&&energy(r)==0,"per-pad zero level silences sample");
        e.noteOn(54,1);check(e.selectedPad()==1,"second physical key selects pad two");
        e.noteOn(76,1);check(e.selectedPad()==23,"last physical key selects pad twenty-four");
        e.noteOn(77,1);check(e.selectedPad()==0,"drum notes wrap after twenty-four semitones");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);Parameters p;p.engine=12;p.attack=.001f;p.sustain=1;e.setParameters(p);e.noteOn(60,1);
        l.resize(4000);r.resize(4000);for(int i=0;i<4000;++i)l[i]=r[i]=.5f*std::sin(i*2*3.141592653589793*700/8000);
        const auto before=watchedAllocations.load();watchAllocations=true;e.process(l.data(),r.data(),l.data(),r.data(),4000);watchAllocations=false;
        check(watchedAllocations.load()==before,"vocoder does not allocate during callback");check(energy(l)>1e-6,"vocoder filterbank transfers external formant energy to carrier");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);e.setSequenceLength(999);for(int i=0;i<999;++i)e.setSequenceStep(i,-1);e.setSequenceStep(998,72,.9f);
        check(e.sequenceLength()==999&&e.sequenceStep(998)==72&&std::abs(e.sequenceVelocity(998)-.9f)<1e-6,"Endless preserves its999th note and velocity");
        Parameters p;p.sequenceOn=true;p.sequencer=2;p.tempo=120;p.attack=.001f;p.release=.01f;e.setParameters(p);e.noteOn(60,1);render(e,l,r,998064);check(e.currentSequenceStep()==998&&std::abs(l.back())>.001f,"Endless reaches and plays its999th programmed step");render(e,l,r,1000);check(e.currentSequenceStep()==0,"Endless wraps its actual sequence length");
        e.allNotesOff();p.sequencer=4;e.setParameters(p);e.noteOn(60,1);e.noteOff(60);render(e,l,r,8000);check(energy(l)>.0001f,"Hold sustains played note across key-up");e.clearHeldNotes();render(e,l,r,1000);check(l.back()==0,"Hold latch can be cleared explicitly");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);Parameters p;p.lfo=4;p.lfoDepth=1;p.lfoTarget=6;p.macro[3]=1;p.attack=.001f;p.sustain=1;e.setParameters(p);e.noteOn(69,1);render(e,l,r,1000);check(energy(l)==0&&energy(r)>.0001f,"LFO can route to pan instead of pitch");
        e.allNotesOff();render(e,l,r,1000);p.lfoTarget=5;p.macro[3]=0;e.setParameters(p);e.noteOn(69,1);render(e,l,r,1000);check(energy(l)==0&&energy(r)==0,"LFO volume destination can fully attenuate voice");
    }
    {
        auto aOwner=std::make_unique<Engine>();auto& a=*aOwner;auto bOwner=std::make_unique<Engine>();auto& b=*bOwner;a.prepare(8000,64);b.prepare(8000,64);std::vector<float> source(8000);for(int i=0;i<8000;++i)source[i]=std::sin(i*2*3.141592653589793*220/8000)*.5f;a.loadSample(source,source,8000);b.loadSample(source,source,8000);
        Parameters p;p.engine=11;p.effect=1;p.effectMix=.5f;p.sampleLoop=true;p.attack=.001f;a.setParameters(p);p.macro[0]=1;p.macro[1]=0;p.macro[2]=0;p.macro[3]=1;b.setParameters(p);a.noteOn(60,1);b.noteOn(60,1);render(a,l,r,8000);std::vector<float> bl,br;render(b,bl,br,8000);check(l==bl&&r==br,"effect parameters are independent of synthesis macros");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);for(int track=0;track<4;++track){e.tracks()[track].left.assign(400,.02f*(track+1));e.tracks()[track].right.assign(400,-.01f*(track+1));e.setTrackLevel(track,1-track*.1f);e.setTrackPan(track,(track-1.5f)/3);}
        e.restoredTape();e.setTrackMute(1,true);e.setPlaying(true);std::array<std::vector<float>,8> channels;std::array<float*,8> stems;for(int i=0;i<8;++i){channels[i].resize(400);stems[i]=channels[i].data();}l.resize(400);r.resize(400);
        const auto before=watchedAllocations.load();watchAllocations=true;e.process(l.data(),r.data(),nullptr,nullptr,400,stems.data());watchAllocations=false;check(watchedAllocations.load()==before,"stem rendering performs no allocations");
        for(int i=0;i<400;++i){float sumL=0,sumR=0;for(int track=0;track<4;++track){sumL+=channels[track*2][i];sumR+=channels[track*2+1][i];}check(std::abs(l[i]-sumL*.65f)<1e-6f&&std::abs(r[i]-sumR*.65f)<1e-6f,"stem sum reproduces uncoloured tape main output before mastergain");}
        check(energy(channels[2])==0&&energy(channels[3])==0,"muted stem is silent");e.setPlaying(false);e.process(l.data(),r.data(),nullptr,nullptr,400,stems.data());for(auto&channel:channels)check(energy(channel)==0,"stems are silent when tape is stopped");
        auto fxOwner=std::make_unique<Engine>();auto& fx=*fxOwner;fx.prepare(8000,64);Parameters p;p.monitorInput=true;p.volume=1;p.effect=1;p.effectMix=0;fx.setParameters(p);fx.setOutputGainImmediately(1);l.assign(400,.2f);r.assign(400,-.1f);fx.process(l.data(),r.data(),l.data(),r.data(),400);check(l[0]==.2f&&r[0]==-.1f,"utility FX can preserve immediate unity dry gain");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);Parameters p;p.engine=14;p.drumMode=false;e.setParameters(p);
        DBoxPad low;low.mix=0;low.wave1=0;low.frequency=.25f;low.envelope1=2;low.cutoff=1;low.smooth=0;DBoxPad high=low;high.frequency=.5f;e.setDboxPad(0,low);e.setDboxPad(1,high);
        check(e.dboxPad(0).frequency==.25f&&e.dboxPad(1).frequency==.5f,"DBox pads preserve independent oscillator parameters");
        e.noteOn(53,1);render(e,l,r,4000);check(e.selectedPad()==0&&energy(l)>.0001f,"DBox is playable from first physical key without drum flag");int lowCross=0;for(int i=801;i<3200;++i)if(l[i-1]<=0&&l[i]>0)++lowCross;e.allNotesOff();render(e,l,r,1000);
        e.noteOn(54,1);render(e,l,r,4000);int highCross=0;for(int i=801;i<3200;++i)if(l[i-1]<=0&&l[i]>0)++highCross;check(std::abs(lowCross/.3-100)<5&&std::abs(highCross/.3-400)<5,"DBox independent pads have distinct tuned oscillator frequencies");e.allNotesOff();render(e,l,r,1000);
        low.envelope1=.02f;low.envelope2=.02f;low.release=.02f;e.setDboxPad(0,low);e.noteOn(53,1);render(e,l,r,4000);double shortTail=0;for(int i=3000;i<4000;++i)shortTail+=l[i]*l[i];e.allNotesOff();render(e,l,r,1000);
        low.envelope1=.8f;e.setDboxPad(0,low);e.noteOn(53,1);render(e,l,r,4000);double longTail=0;for(int i=3000;i<4000;++i)longTail+=l[i]*l[i];check(longTail>shortTail*1000&&longTail>.001,"DBox per-oscillator envelope changes audible decay");e.allNotesOff();render(e,l,r,1000);
        low.gain=0;e.setDboxPad(0,low);e.noteOn(53,1);render(e,l,r,1000);check(energy(l)==0&&energy(r)==0,"DBox per-pad transient gain can mute its voice");e.allNotesOff();render(e,l,r,1000);
        low.gain=1;e.setDboxPad(0,low);p.effect=1;p.effectMix=1;e.setParameters(p);e.noteOn(53,1);render(e,l,r,4000);double early=0,late=0;for(int i=0;i<1000;++i)early+=l[i]*l[i];for(int i=2500;i<4000;++i)late+=l[i]*l[i];check(early<1e-9&&late>.001,"DBox feeds the shared effects module");
    }
    {
        auto aOwner=std::make_unique<Engine>();auto& a=*aOwner;auto bOwner=std::make_unique<Engine>();auto& b=*bOwner;a.prepare(8000,64);b.prepare(8000,64);Parameters p;p.engine=14;a.setParameters(p);b.setParameters(p);DBoxPad pad;pad.mix=.4f;pad.wave1=.3f;pad.wave2=.4f;pad.crossMod=0;pad.envelope1=.3f;a.setDboxPad(0,pad);pad.crossMod=.8f;b.setDboxPad(0,pad);a.noteOn(53,1);b.noteOn(53,1);render(a,l,r,2000);std::vector<float> bl,br;render(b,bl,br,2000);double difference=0;for(int i=0;i<2000;++i)difference+=std::abs(l[i]-bl[i]);check(difference>.1,"DBox dual-oscillator cross-modulation changes timbre");
    }
    {
        auto topOwner=std::make_unique<Engine>();auto& top=*topOwner;auto bottomOwner=std::make_unique<Engine>();auto& bottom=*bottomOwner;top.prepare(8000,64);bottom.prepare(8000,64);Parameters p;p.engine=1;p.macro[1]=0;p.attack=.001f;p.sustain=1;p.sequenceOn=true;p.sequencer=6;p.seqGate=1;top.setParameters(p);bottom.setParameters(p);top.setSketchLength(2);bottom.setSketchLength(2);top.setSketchPoint(0,0,0);top.setSketchPoint(1,1,0);bottom.setSketchPoint(0,0,1);bottom.setSketchPoint(1,1,1);top.noteOn(60,1);bottom.noteOn(60,1);render(top,l,r,800);int high=0;for(int i=101;i<800;++i)if(l[i-1]<=0&&l[i]>0)++high;render(bottom,l,r,800);int low=0;for(int i=101;i<800;++i)if(l[i-1]<=0&&l[i]>0)++low;check(high>low*3,"Sketch Y path changes played pitch rather than generic pattern");
        top.clearHeldNotes();render(top,l,r,500);top.setSketchPoint(0,.2f,.5f);top.setSketchPoint(1,.8f,.5f);top.setSequenceLength(4);top.noteOn(60,1);render(top,l,r,500);check(energy(l)==0,"Sketch X excludes time outside drawn path");render(top,l,r,1100);check(energy(l)>.0001f,"Sketch X enters and plays drawn path");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);Parameters p;p.engine=1;p.macro[1]=0;p.attack=.001f;p.sustain=1;p.sequenceOn=true;p.sequencer=3;p.seqGate=1;e.setParameters(p);for(int bank=0;bank<2;++bank){e.setFingerLength(bank,4);for(int step=0;step<32;++step)e.setFingerStep(bank,step,-1);}e.setFingerStep(0,0,60,.9f);e.setFingerStep(1,0,72,.7f);check(e.fingerStep(0,0)==60&&e.fingerStep(1,0)==72,"Finger banks retain independent actual notes");e.noteOn(53,1);render(e,l,r,800);int low=0;for(int i=101;i<800;++i)if(l[i-1]<=0&&l[i]>0)++low;e.clearHeldNotes();render(e,l,r,500);e.noteOn(54,1);check(e.fingerPattern()==1,"Finger view follows key-selected bank");render(e,l,r,800);int high=0;for(int i=101;i<800;++i)if(l[i-1]<=0&&l[i]>0)++high;check(high>low*1.7,"Finger key chooses bank notes without transposing shared stream");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);Parameters p;p.sequenceOn=true;p.sequencer=1;p.tempo=120;p.seqSwing=.5f;p.seqGate=1;p.attack=.001f;p.release=.005f;e.setParameters(p);e.noteOn(60,1);render(e,l,r,1200);check(e.currentSequenceStep()==0,"swing lengthens first sixteenth");render(e,l,r,400);check(e.currentSequenceStep()==1,"swing starts delayed offbeat");render(e,l,r,400);check(e.currentSequenceStep()==2,"swing preserves the two-step beat duration");
        e.clearHeldNotes();render(e,l,r,500);p.seqSwing=0;p.seqGate=.05f;e.setParameters(p);e.noteOn(60,1);render(e,l,r,400);double tail=0;for(int i=200;i<400;++i)tail+=l[i]*l[i];check(tail<1e-9,"short sequence gate releases note before next tick");e.clearHeldNotes();render(e,l,r,500);p.seqHold=true;e.setParameters(p);e.noteOn(60,1);e.noteOff(60);check(e.heldNoteCount()==1&&e.heldNote(0)==60&&e.heldNote(1)==-1,"sequence hold retains latch and exposes truthful held notes");render(e,l,r,2400);check(e.currentSequenceStep()>=2,"latched pattern continues after key-up");e.clearHeldNotes();check(e.heldNoteCount()==0,"clear releases all latched notes");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);Parameters p;p.sequenceOn=true;p.sequencer=7;p.seqHold=true;p.attack=.001f;p.release=.02f;p.sequencerMacro[0]=.7f;p.sequencerMacro[1]=.8f;p.sequencerMacro[3]=.8f;e.setParameters(p);e.noteOn(60,1);e.noteOn(64,.8f);e.noteOff(60);render(e,l,r,32000);check(e.tombolaTriggerCount()>0&&energy(l)>.000001f,"Tombola wall collisions trigger audible notes");auto balls=e.tombolaBalls();int active=0;const double angle=e.tombolaRotation(),limit=.44*std::cos(3.141592653589793/e.tombolaSides())-.018;for(const auto&ball:balls)if(ball.active){++active;check(std::isfinite(ball.x)&&std::isfinite(ball.vx)&&ball.x>=0&&ball.x<=1&&ball.y>=0&&ball.y<=1,"Tombola balls stay finite and within display bounds");for(int wall=0;wall<e.tombolaSides();++wall){double a=angle+wall*2*3.141592653589793/e.tombolaSides();check((ball.x-.5)*std::cos(a)+(ball.y-.5)*std::sin(a)<=limit+.0001,"Tombola balls remain inside rotating polygon");}}check(active==2,"Tombola hold preserves released ball");
        p.sequenceOn=false;e.setParameters(p);render(e,l,r,8000);auto paused=e.tombolaBalls();for(size_t i=0;i<balls.size();++i)check(paused[i].x==balls[i].x&&paused[i].y==balls[i].y,"disabled Tombola pauses simulation");p.sequenceOn=true;e.setParameters(p);render(e,l,r,1000);auto resumed=e.tombolaBalls();check(resumed[0].x!=paused[0].x||resumed[0].y!=paused[0].y,"Tombola resumes existing physical state");e.clearHeldNotes();for(const auto&ball:e.tombolaBalls())check(!ball.active,"clear removes Tombola balls");
    }
    {
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(std::numeric_limits<double>::quiet_NaN(),64);check(e.sampleRate()==44100,"nonfinite sample rate uses safe default");e.loadSample(std::vector<float>(100,.1f),{},std::numeric_limits<double>::infinity());check(e.loadedSampleRate()==44100,"nonfinite sample rate cannot cause invalid sample allocation");e.setLoop(std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity());check(e.loopStart()==0&&e.loopEnd()==2,"nonfinite loop endpoints use safe bounds");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,128);Parameters p;p.engine=11;p.attack=.1f;p.decay=.1f;p.sustain=.25f;p.release=.1f;p.volume=1;e.loadSample(std::vector<float>(8000,.5f),{},8000);e.setParameters(p);e.setOutputGainImmediately(1);e.noteOn(60,1);render(e,l,r,2000);
        check(std::abs(l[399]-.25)<.0002,"ADSR attack reaches half level at half attack time");check(std::abs(l[799]-.5)<.0002,"ADSR attack reaches peak at configured time");check(std::abs(l[1599]-.125)<.0002&&std::abs(l[1999]-.125)<.0002,"ADSR decay reaches and holds sustain");e.noteOff(60);render(e,l,r,400);check(std::abs(l[399]-.0625)<.0002,"ADSR release falls from current sustain level");render(e,l,r,500);check(std::abs(l.back())<1e-5,"ADSR release ends in silence");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,128);Parameters p;p.engine=11;p.attack=.5f;p.sustain=1;p.voiceMode=2;p.glideTime=.1f;p.volume=1;e.loadSample(std::vector<float>(16000,.5f),{},8000);e.setParameters(p);e.setOutputGainImmediately(1);e.noteOn(60,1);render(e,l,r,400);float before=l.back();e.noteOn(72,1);render(e,l,r,1);check(e.activeVoiceCount()==1&&l[0]>=before,"legato preserves envelope without layering voices");check(e.currentVoicePitch()>60&&e.currentVoicePitch()<61,"portamento begins from held pitch");render(e,l,r,800);check(e.currentVoicePitch()>67&&e.currentVoicePitch()<69,"portamento moves toward target over its time constant");e.noteOff(72);render(e,l,r,1);check(e.activeVoiceCount()==1,"mono last-key priority returns to a held key");e.noteOff(60);render(e,l,r,4000);check(e.activeVoiceCount()==0,"mono fallback releases after last key");
        p.voiceMode=0;e.setParameters(p);e.noteOn(60,1);e.noteOn(64,1);check(e.activeVoiceCount()==2,"poly mode layers independent notes");p.voiceMode=1;e.setParameters(p);e.noteOn(67,1);check(e.activeVoiceCount()==1,"mono mode consolidates voice allocation");e.allNotesOff();render(e,l,r,100);p.voiceMode=3;e.setParameters(p);e.noteOn(60,1);check(e.activeVoiceCount()==4,"unison mode allocates detuned voice stack");render(e,l,r,800);check(energy(l)>0,"unison stack produces bounded audio");
    }

    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);
        Parameters p;p.lfo=4;p.lfoDepth=1;p.macro[3]=1;
        for(int target=0;target<20;++target){p.lfoTarget=target;e.setParameters(p);e.noteOn(60,1);render(e,l,r,64);check(e.modulationAt(target)==1,"all twenty destination routes receive modulation");for(float sample:l)check(std::isfinite(sample),"all destination renders finite");e.allNotesOff();}
        p.lfo=2;p.midiLfoTargets={{0,6,12,19}};p.midiLfoDepth={{1,-1,.5f,.25f}};e.setParameters(p);
        for(int cc=1;cc<=4;++cc)e.setMidiCC(cc,1);render(e,l,r,1);
        check(e.modulationAt(0)==1&&e.modulationAt(6)==-1&&e.modulationAt(12)==.5f&&e.modulationAt(19)==.25f,"four independent MIDI routes including inversion");
        p.midiLfoDepth.fill(0);p.lfoTarget=7;e.setParameters(p);e.setModWheel(0);render(e,l,r,1);check(e.modulationAt(7)==-1,"legacy modwheel route remains compatible");
        p.lfo=3;p.lfoSync=true;p.tempo=120;p.lfoDivision=4;p.lfoRetrigger=true;e.setParameters(p);
        for(int shape=0;shape<6;++shape){p.lfoWaveform=shape;e.setParameters(p);e.noteOn(60,1);check(e.lfoPhasePosition()==0,"note retriggers LFO phase");render(e,l,r,1000);check(std::abs(e.lfoPhasePosition()-.25)<1e-8,"tempo sync advances quarter cycle at 120 BPM");check(std::abs(e.modulationAt(7))<=1,"waveforms remain bounded");e.allNotesOff();}
        p.lfo=1;p.elementSource=3;p.elementManual=.8f;e.setParameters(p);render(e,l,r,100);check(std::abs(e.modulationAt(7)-.6f)<1e-6,"Element manual substitute uses its own value");
        p.elementSource=0;e.setParameters(p);render(e,l,r,100);check(e.modulationAt(7)==-1,"Element input envelope is independent of manual value");
        std::vector<float> input(1000,.8f);l.resize(1000);r.resize(1000);e.process(l.data(),r.data(),input.data(),input.data(),1000);check(e.modulationAt(7)>.59f,"Element follows actual input envelope");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.engine=11;p.volume=1;p.attack=.1f;p.decay=.1f;p.sustain=.25f;p.release=.1f;p.envelopeCurve=1;p.effectMix=0;
        e.loadSample(std::vector<float>(16000,.5f),{},8000);e.setParameters(p);e.setOutputGainImmediately(1);e.noteOn(60,1);render(e,l,r,2400);
        for(int i=1;i<799;++i)check(l[i]>=l[i-1],"exponential attack monotonic");
        check(std::abs(l[799]-.5f)<.0001f,"exponential attack reaches deterministic peak");
        check(std::abs(l[1601]-.125f)<.0001f,"exponential decay reaches sustain");
        e.noteOff(60);render(e,l,r,802);for(int i=1;i<799;++i)check(l[i]<=l[i-1],"exponential release monotonic");check(e.activeVoiceCount()==0,"exponential release terminates at endpoint");
    }

    {
        auto renderRoute=[&](int target,float depth){
            auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;
            p.engine=1;p.effectMacro[2]=.2f;p.macro[2]=.2f;p.lfo=4;p.macro[3]=.7f;p.lfoDepth=depth;p.lfoTarget=target;p.volume=.8f;p.effect=1;p.effectMix=.6f;
            p.attack=.06f;p.decay=.08f;p.sustain=.45f;p.release=.08f;e.setParameters(p);e.noteOn(60,1);
            std::vector<float> a,b;render(e,a,b,5000);e.noteOff(60);std::vector<float> tail,other;render(e,tail,other,1600);a.insert(a.end(),tail.begin(),tail.end());a.insert(a.end(),b.begin(),b.end());return a;
        };
        for(int target=0;target<20;++target){if(target==18)continue;const auto a=renderRoute(target,0),b=renderRoute(target,.8f);double difference=0;for(size_t i=0;i<a.size();++i)difference+=std::abs(a[i]-b[i]);if(difference<=.00001)std::cerr<<"target "<<target<<" difference "<<difference<<"\n";check(difference>.00001,"source, envelope, FX and gain destinations change rendered audio");}
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.lfo=4;p.lfoDepth=1;p.lfoTarget=18;p.macro[3]=1;p.monitorInput=true;e.setParameters(p);
        e.setRecording(true);l.assign(8000,.1f);r.assign(8000,.1f);e.process(l.data(),r.data(),l.data(),r.data(),8000);e.setRecording(false);e.seek(0);e.setPlaying(true);render(e,l,r,400);check(std::abs(e.positionSeconds()-.1)<1e-7,"tape-speed destination changes transport advance");
    }

    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.engine=11;p.attack=.001f;p.decay=.001f;p.sustain=1;p.lfo=5;p.lfoDepth=1;p.lfoTarget=6;p.volume=1;
        e.loadSample(std::vector<float>(16000,.5f),{},8000);e.setParameters(p);e.setOutputGainImmediately(1);e.noteOn(60,.25f);e.noteOn(64,1);render(e,l,r,100);
        check(l.back()>.01f&&r.back()>l.back()*4,"velocity pan is evaluated independently for simultaneous voices");
        p.lfo=1;p.lfoTarget=0;p.elementSource=1;e.setParameters(p);render(e,l,r,10);check(e.modulationAt(0)>.99f,"Element uses maximum active voice envelope");
        p.elementSource=2;e.setParameters(p);render(e,l,r,10);check(std::abs(e.modulationAt(0)-(2*std::max(std::abs(l[8]),std::abs(r[8]))-1))<1e-5,"Element output follows actual previous main signal");
    }
    {
        // A constant sample at its root pitch isolates source and output gain.
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);
        e.loadSample(std::vector<float>(16000,.2f),std::vector<float>(16000,-.1f),8000);
        Parameters p;p.engine=11;p.volume=1;p.attack=.001f;p.decay=.001f;p.sustain=1;p.effect=0;e.setParameters(p);e.setOutputGainImmediately(1);e.noteOn(60,1);
        render(e,l,r,4000);check(std::abs(l.back()-.2f)<1e-5f&&std::abs(r.back()+.1f)<1e-5f,"sampler preserves stereo source level at unity gain");
        p.volume=2;e.setParameters(p);render(e,l,r,8000);check(std::abs(l.back()-.4f)<1e-4f&&std::abs(r.back()+.2f)<1e-4f,"master offers six decibels of clean gain above unity");
        p.volume=0;e.setParameters(p);render(e,l,r,8000);check(std::abs(l.back())<.0002f,"master still fades to silence");
    }
    std::cout<<"PASS: tuning, release, recording, stereo, overdub, undo/redo, lift/drop, splice, resampling, forward/reverse loops, export, mute/solo, sampler slices, engines/effects, sequencer\n";
}
