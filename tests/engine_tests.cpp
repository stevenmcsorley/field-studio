#include "Engine.h"
#include <algorithm>
#include <atomic>
#include <new>
#include <memory>
#include <thread>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace field;
namespace {
thread_local bool watchAllocations=false;
std::atomic<size_t> watchedAllocations{0};
std::atomic<size_t> watchedDeallocations{0};
}
void* operator new(std::size_t n){if(watchAllocations)++watchedAllocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{if(p&&watchAllocations)++watchedDeallocations;std::free(p);}
void operator delete[](void* p)noexcept{if(p&&watchAllocations)++watchedDeallocations;std::free(p);}
void operator delete(void* p,std::size_t)noexcept{if(p&&watchAllocations)++watchedDeallocations;std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{if(p&&watchAllocations)++watchedDeallocations;std::free(p);}
namespace {
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
float energy(const std::vector<float>& v){double total=0;for(float x:v)total+=x*x;return static_cast<float>(total/std::max<size_t>(1,v.size()));}
void render(Engine& e,std::vector<float>& l,std::vector<float>& r,int frames){l.resize(frames);r.resize(frames);const auto allocations=watchedAllocations.load(),deallocations=watchedDeallocations.load();watchAllocations=true;
    e.process(l.data(),r.data(),nullptr,nullptr,frames);watchAllocations=false;
    check(watchedAllocations.load()==allocations,"audio render performs no heap allocation");
    check(watchedDeallocations.load()==deallocations,"audio render performs no heap deallocation");}
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
        check(e.tracks()[0].audio.size()==800,"recording writes exact frame count");
        check(std::abs(e.tracks()[0].audio.read(0,20)-.2f)<1e-6,"recording preserves left input");
        check(std::abs(e.tracks()[0].audio.read(1,20)+.1f)<1e-6,"recording preserves stereo input");
        check(std::abs(e.positionSeconds()-.1)<1e-9,"transport advances by audio time");
        e.rewind();e.setRecording(true);l.assign(800,.1f);r.assign(800,.1f);e.process(l.data(),r.data(),l.data(),r.data(),800);e.setRecording(false);e.setPlaying(false);
        check(std::abs(e.tracks()[0].audio.read(0,100)-.3f)<1e-6,"overdub adds to existing audio");
        e.undo();check(std::abs(e.tracks()[0].audio.read(0,100)-.2f)<1e-6,"undo restores previous overdub");e.redo();check(std::abs(e.tracks()[0].audio.read(0,100)-.3f)<1e-6,"redo restores overdub");
        e.lift(.025,.05);check(e.tracks()[0].audio.read(0,250)==0,"lift silences source without ripple");check(e.tracks()[0].audio.size()==800,"lift preserves tape timeline");
        e.selectTrack(1);e.seek(.05);e.drop();check(e.tracks()[1].audio.size()==600,"drop places copied span at playhead");check(std::abs(e.tracks()[1].audio.read(0,450)-.3f)<1e-6,"clipboard crosses tracks");
        e.seek(.06);e.split();check(e.tracks()[1].cuts.size()>=3,"split records splice boundary");
        e.prepare(16000,128);check(std::abs(e.recordedSeconds()-.1)<1e-9,"sample-rate change preserves tape duration");check(std::abs(e.tracks()[0].audio.read(0,100)-.3f)<1e-6,"sample-rate change preserves tape samples");
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
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,128);e.tracks()[0].audio.assign(800,.25f,-.15f);e.restoredTape();
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
        auto eOwner=std::make_unique<Engine>();auto& e=*eOwner;e.prepare(8000,64);for(int track=0;track<4;++track){e.tracks()[track].audio.assign(400,.02f*(track+1),-.01f*(track+1));e.setTrackLevel(track,1-track*.1f);e.setTrackPan(track,(track-1.5f)/3);}
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

    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);
        for(int track=0;track<4;++track){auto& t=e.tracks()[track];t.audio.assign(80,float(track+1)*.1f,float(track+1)*.1f);t.cuts={.002,.004,.006};}
        e.copyTracks(5,.002,.006);check(!e.canUndo(),"copy is nondestructive and does not add history");
        e.liftTracks(5,.002,.006);check(e.tracks()[0].audio.read(0,20)==0&&e.tracks()[2].audio.read(0,20)==0&&e.tracks()[1].audio.read(0,20)==.2f,"masked lift leaves unselected tracks untouched");
        e.undo();check(e.tracks()[0].audio.read(0,20)==.1f&&e.tracks()[2].audio.read(0,20)==.3f,"one undo restores entire masked edit");e.redo();check(e.tracks()[0].audio.read(0,20)==0&&e.tracks()[2].audio.read(0,20)==0,"one redo reapplies entire masked edit");
        e.seek(.008);e.dropTracks(1);check(e.tracks()[1].audio.read(0,65)==.1f&&e.tracks()[3].audio.read(0,65)==.3f,"drop preserves relative masked track spacing");e.undo();check(e.tracks()[1].audio.size()==80&&e.tracks()[3].audio.size()==80,"masked drop undoes atomically including lengths");
        e.joinTracks(10,0,.01);check(e.tracks()[1].cuts.empty()&&e.tracks()[3].cuts.empty(),"join removes interior cuts on selected tracks");e.undo();check(e.tracks()[1].cuts.size()==3&&e.tracks()[3].cuts.size()==3,"join history restores all markers");
        e.selectTrack(1);e.copyTracks(0,0,.002);e.selectTrack(2);e.seek(.008);e.drop();check(e.tracks()[2].audio.read(0,65)==.2f,"mask zero follows selected and legacy drop relocates single track");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);auto& t=e.tracks()[0];t.audio.resize(80);for(int i=0;i<80;++i)t.audio.write(i,float(i),float(i));t.cuts={.002,.004};
        e.slideTracks(1,.002,.006,.002);check(t.audio.read(0,16)==0&&t.audio.read(0,31)==0&&t.audio.read(0,32)==16&&t.audio.read(0,63)==47&&t.audio.read(0,64)==64,"overlapping slide preserves source snapshot and nonripple neighbours");
        e.undo();check(e.tracks()[0].audio.read(0,16)==16&&e.tracks()[0].audio.read(0,63)==63,"slide undo restores overwritten destination");
        e.slideTracks(1,.002,.006,-.002);check(e.tracks()[0].audio.read(0,0)==16&&e.tracks()[0].audio.read(0,31)==47&&e.tracks()[0].audio.read(0,32)==0,"slide left is overlap safe");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);
        for(int i=0;i<3;++i){e.tracks()[i].audio.assign(80,.2f,.4f);}e.setTrackLevel(0,.5f);e.setTrackPan(0,1);e.setTrackMute(2,true);
        e.copyTracks(7,0,.01);e.seek(.02);e.mergeClipboard(3);check(std::abs(e.tracks()[3].audio.read(0,170)-.2f)<1e-6&&std::abs(e.tracks()[3].audio.read(1,170)-.6f)<1e-6,"merge clipboard applies source mixer levels pan and mute");e.undo();check(e.tracks()[3].audio.empty(),"merge is one undo operation");
        e.bounceTracks(7,1,0,.01,true);check(e.tracks()[0].audio.read(0,20)==0&&e.tracks()[2].audio.read(0,20)==0&&std::abs(e.tracks()[1].audio.read(1,20)-.6f)<1e-6,"bounce captures overlapping destination before clearing sources");e.undo();check(e.tracks()[0].audio.read(0,20)==.2f&&e.tracks()[1].audio.read(1,20)==.4f&&e.tracks()[2].audio.read(0,20)==.2f,"bounce and cleared sources restore atomically");
        e.seek(.02);e.mergeClipboard(3);check(std::abs(e.tracks()[3].audio.read(1,170)-.6f)<1e-6,"bounce preserves user's prior clipboard");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.tempo=120;p.volume=1;p.monitorInput=true;e.setParameters(p);e.setOutputGainImmediately(1);e.seek(.1);e.setCountInBeats(2);e.setRecording(true);
        render(e,l,r,4000);check(e.recording()&&std::abs(e.countInRemainingBeats()-1)<1e-8&&std::abs(e.positionSeconds()-.1)<1e-8&&e.tracks()[0].audio.empty(),"count-in arms record without advancing or writing tape");check(energy(l)>.00001,"count-in clicks audible when metronome disabled");
        e.setCountInBeats(2);render(e,l,r,4000);check(e.countInRemainingBeats()==0&&e.tracks()[0].audio.empty(),"count-in is sample accurate and setting updates do not restart it");
        l.assign(80,.2f);r.assign(80,-.1f);const auto allocationCount=watchedAllocations.load();watchAllocations=true;e.process(l.data(),r.data(),l.data(),r.data(),80);watchAllocations=false;check(watchedAllocations.load()==allocationCount,"record transition performs no callback allocations");check(e.tracks()[0].audio.size()==880&&std::abs(e.tracks()[0].audio.read(0,800)-.2f)<1e-6,"recording begins at frozen head after countdown");
        e.setPlaying(false);e.setRecording(true);render(e,l,r,100);e.setPlaying(false);check(e.countInRemainingBeats()==0&&!e.recording(),"stop cancels armed count-in");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.tempo=120;p.volume=1;e.setParameters(p);e.setOutputGainImmediately(1);e.tracks()[0].audio.assign(16000,.2f,.2f);e.seek(.5);e.setPlaying(true);
        e.setTapeBrake(true);render(e,l,r,400);check(e.positionSeconds()==.5&&energy(l)==0,"momentary break freezes and silences tape");e.setTapeBrake(false);e.setTapeReverse(true);render(e,l,r,400);check(std::abs(e.positionSeconds()-.45)<1e-8,"momentary reverse changes tape direction");e.setTapeReverse(false);
        e.setTapeRoll(true,.25f);render(e,l,r,2500);check(e.positionSeconds()>=.45&&e.positionSeconds()<.575,"tempo roll repeats bounded window from press point");e.setTapeRoll(false);const double before=e.positionSeconds();render(e,l,r,400);check(std::abs(e.positionSeconds()-before-.05)<1e-8,"roll release resumes continuous tape advance");e.setTapeBrake(true);e.setPlaying(false);e.setPlaying(true);render(e,l,r,80);check(e.positionSeconds()>before+.05,"stop clears momentary tape tricks");
    }

    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.volume=1;p.monitorInput=true;p.tapeRecordLevel=2;p.tapeRecordPan=.5f;e.setParameters(p);e.setOutputGainImmediately(1);e.setRecording(true);
        l.assign(80,.2f);r.assign(80,.1f);e.process(l.data(),r.data(),l.data(),r.data(),80);e.setPlaying(false);check(std::abs(e.tracks()[0].audio.read(0,0)-.2f)<1e-6&&std::abs(e.tracks()[0].audio.read(1,0)-.2f)<1e-6,"record level and pan affect captured stereo only");check(std::abs(l[0]-.2f)<1e-6&&std::abs(r[0]-.1f)<1e-6,"record balance does not alter monitored source");
        const auto original=e.tracks()[0].audio.exportChannel(0);e.liftTracks(0,std::numeric_limits<double>::quiet_NaN(),1);e.slideTracks(0,0,1,std::numeric_limits<double>::infinity());check(e.tracks()[0].audio.exportChannel(0)==original,"malformed edit bounds leave tape intact");
        e.setCountInBeats(2);e.setRecording(true);e.setTapeBrake(true);e.prepare(8000,64);check(!e.recording()&&e.countInRemainingBeats()==0,"prepare clears armed recording state");e.setRecording(true);e.restoredTape();check(!e.recording()&&e.countInRemainingBeats()==0,"tape restore clears armed recording state");
    }

    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.sequenceOn=true;p.sequencer=2;p.tempo=120;p.attack=.001f;p.release=.001f;p.seqGate=1;e.setParameters(p);e.setSequenceLength(8);for(int i=0;i<8;++i)e.setSequenceStep(i,60+i,1);e.seek(.75);e.noteOn(60,1);render(e,l,r,200);
        check(e.currentSequenceStep()==0&&e.currentVoicePitch()==60,"held sequence begins at step zero");e.setSongPosition(13);check(e.currentSequenceStep()==5&&e.positionSeconds()==.75&&e.heldNoteCount()==1,"MIDI song position changes sequencer without tape seek or held-key loss");
        render(e,l,r,999);check(e.activeVoiceCount()==0&&e.currentSequenceStep()==5,"song position itself creates no new note and clears prior sequence gates");render(e,l,r,1);check(e.currentSequenceStep()==5&&e.currentVoicePitch()==65,"next natural sixteenth plays the requested song-position step");render(e,l,r,1000);check(e.currentSequenceStep()==6,"sequencer advances normally after positioned step");render(e,l,r,50);check(e.currentVoicePitch()==66,"next step note follows positioned step after prior release");
        e.allNotesOff();e.setSongPosition(-20);check(e.currentSequenceStep()==0,"negative song position clamps safely");e.setSongPosition(20000);check(e.currentSequenceStep()==7,"song position clamps to MIDI fourteen-bit domain");e.rewind();e.noteOn(60,1);check(e.currentSequenceStep()==0,"rewind clears pending song position");
        p.sequencer=3;e.setParameters(p);e.setSongPosition(63);check(e.currentSequenceStep()==31,"Finger song position uses fixed thirty-two-step clock");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.volume=1;p.metronome=true;p.tempo=120;e.setParameters(p);e.setOutputGainImmediately(1);e.setPlaying(true);render(e,l,r,1000);const float full=energy(l);check(full>0,"tape metronome produces reference clicks");
        p.metronomeLevel=.25f;e.setParameters(p);e.seek(0);e.setOutputGainImmediately(1);render(e,l,r,1000);check(std::abs(energy(l)/full-.0625f)<1e-5,"metronome level scales tape clicks");
        e.setPlaying(false);p.metronome=false;p.metronomeLevel=1;e.setParameters(p);e.setCountInBeats(1);e.setRecording(true);render(e,l,r,1000);const float countFull=energy(l);e.setPlaying(false);p.metronomeLevel=.25f;e.setParameters(p);e.setRecording(true);render(e,l,r,1000);check(std::abs(energy(l)/countFull-.0625f)<1e-5,"metronome level also scales count-in clicks");
        p.metronomeLevel=0;e.setParameters(p);render(e,l,r,1000);check(energy(l)==0,"zero metronome level mutes count-in");p.metronomeLevel=2;e.setParameters(p);check(e.parameters().metronomeLevel==1,"metronome level clamps to unity");
    }

    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.sequenceOn=true;p.sequencer=1;p.tempo=120;p.seqGate=.5f;p.voiceMode=3;e.setParameters(p);e.noteOn(60,.8f);e.beginMidiBlock();check(e.midiEventCount()==1&&e.midiEvents()[0].on&&e.midiEvents()[0].note==60&&e.midiEvents()[0].frame==0,"outside-block sequencer trigger emits one MIDI note independent of unison voices");
        render(e,l,r,500);check(e.midiEventCount()==2&&!e.midiEvents()[1].on&&e.midiEvents()[1].frame==499,"sequencer gate off has sample-accurate block offset");render(e,l,r,500);check(e.midiEventCount()==3&&e.midiEvents()[2].on&&e.midiEvents()[2].frame==999,"MIDI frame offsets accumulate across process chunks");
        e.allNotesOff();e.beginMidiBlock();check(e.midiEventCount()==1&&!e.midiEvents()[0].on&&e.midiEvents()[0].note==60&&e.midiEvents()[0].frame==0,"all-notes-off schedules active sequencer note release next block");
        p.sequenceOn=false;e.setParameters(p);e.beginMidiBlock();e.noteOn(64,1);render(e,l,r,64);check(e.midiEventCount()==0,"manual incoming notes are not duplicated by sequencer MIDI output");e.allNotesOff();
        p.sequenceOn=true;e.setParameters(p);e.beginMidiBlock();for(int i=0;i<150;++i){e.noteOn(60,1);e.allNotesOff();}e.beginMidiBlock();check(e.midiEventOverflowed()&&e.midiEventCount()==1&&e.midiEvents()[0].note==-1&&!e.midiEvents()[0].on,"pending queue overflow emits explicit panic instead of dropping note offs");
        e.beginMidiBlock();e.noteOn(60,1);e.beginMidiBlock();render(e,l,r,300000);check(e.midiEventOverflowed()&&e.midiEventCount()==1&&e.midiEvents()[0].note==-1,"render queue overflow emits panic without allocation");e.allNotesOff();e.beginMidiBlock();e.beginMidiBlock();check(!e.midiEventOverflowed()&&e.midiEventCount()==0,"next MIDI block recovers from overflow");
        e.beginMidiBlock();render(e,l,r,25);e.noteOn(60,1);render(e,l,r,1);check(e.midiEventCount()==1&&e.midiEvents()[0].on&&e.midiEvents()[0].frame==25,"control-thread trigger between chunks emits at next process offset");e.allNotesOff();e.beginMidiBlock();
        e.noteOn(60,1);e.beginMidiBlock();e.setSongPosition(3);e.beginMidiBlock();check(e.midiEventCount()==1&&!e.midiEvents()[0].on,"song-position change emits previous gate release without new note");e.noteOn(64,1);render(e,l,r,1000);e.setPlaying(false);e.beginMidiBlock();check(e.midiEventCount()>0&&!e.midiEvents()[0].on,"transport stop releases active sequencer MIDI gates");
    }

    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.engine=11;p.attack=.001f;p.decay=.001f;p.sustain=1;p.release=.005f;p.volume=1;p.sampleLoop=true;e.setParameters(p);e.setOutputGainImmediately(1);
        auto oldAsset=e.registerPreparedSample(Engine::prepareSample(std::vector<float>(8000,.25f),{},8000));check(e.installPreparedSample(oldAsset),"prepared sample installs");e.noteOn(60,1);render(e,l,r,100);check(std::abs(l.back()-.25f)<1e-6,"first sample voice reads immutable asset");
        auto newAsset=e.registerPreparedSample(Engine::prepareSample(std::vector<float>(8000,-.5f),{},8000));check(e.collectRetiredSamples()==0,"uninstalled prepared handle remains producer-pinned");const auto allocations=watchedAllocations.load(),deallocations=watchedDeallocations.load();watchAllocations=true;const bool installed=e.installPreparedSample(newAsset);watchAllocations=false;check(installed&&watchedAllocations.load()==allocations&&watchedDeallocations.load()==deallocations,"audio sample install allocates and frees nothing");
        check(e.sampleLeft()[0]==-.5f&&e.loadedSampleRate()==8000,"current sample getters reflect installed asset");check(e.collectRetiredSamples()==0&&e.retainedSampleAssets()==2,"old held voice prevents retirement");render(e,l,r,10000);check(std::abs(l.back()-.25f)<1e-6,"old looping voice retains old audio through long sample turnover");
        e.noteOn(64,.25f);render(e,l,r,100);check(std::abs(l.back()-.125f)<1e-5,"new notes use new sample while old notes retain old sample");e.noteOff(60);render(e,l,r,100);check(std::abs(l.back()+.125f)<1e-5,"old sample release leaves new sample playing");check(e.collectRetiredSamples()==1&&e.retainedSampleAssets()==1,"worker reclaims retired asset only after last old voice ends");
        auto discarded=e.registerPreparedSample(Engine::prepareSample({.9f},{},8000));e.discardPreparedSample(discarded);check(e.collectRetiredSamples()==1,"cancelled producer pin retires on worker");
        auto third=e.registerPreparedSample(Engine::prepareSample(std::vector<float>(8000,.1f),{},16000));e.installPreparedSample(third);e.allNotesOff();render(e,l,r,100);check(e.collectRetiredSamples()==1,"all-notes-off releases retained assets without callback deletion");
        auto moved=std::make_unique<Engine>();*moved=std::move(e);moved->noteOn(60,1);render(*moved,l,r,100);check(std::abs(l.back()-.1f)<1e-5,"engine move preserves registry and active sample pointers");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.engine=11;p.volume=1;p.attack=.001f;p.sustain=1;p.release=.005f;e.setParameters(p);e.setOutputGainImmediately(1);
        auto asset=e.registerPreparedSample(Engine::prepareSample(std::vector<float>(16,.2f),{},8000));e.installPreparedSample(asset);e.noteOn(60,1);auto replacement=e.registerPreparedSample(Engine::prepareSample({.3f},{},8000));e.installPreparedSample(replacement);render(e,l,r,64);check(e.activeVoiceCount()==0&&e.collectRetiredSamples()==1,"natural sample endpoint releases retired asset");
        e.loadSample({std::numeric_limits<float>::infinity(),.2f},{-.1f},std::numeric_limits<double>::quiet_NaN());check(e.sampleLeft()[0]==-4&&e.sampleRight()[1]==-.1f&&e.loadedSampleRate()==44100,"prepared assets retain input sanitation and stereo fallback");
    }

    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.engine=11;p.volume=.5f;p.attack=.001f;p.release=.005f;e.setParameters(p);
        std::array<Engine::PreparedSample*,64> handles{};for(auto& handle:handles)handle=e.registerPreparedSample(Engine::prepareSample(std::vector<float>(512,.1f),{},8000));
        std::atomic<bool> finished{false};std::thread collector([&]{while(!finished.load(std::memory_order_acquire)){e.collectRetiredSamples();std::this_thread::yield();}});
        for(auto* handle:handles){const auto frees=watchedDeallocations.load();watchAllocations=true;const bool installed=e.installPreparedSample(handle);e.noteOn(60,1);watchAllocations=false;check(installed&&watchedDeallocations.load()==frees,"concurrent retirement never deletes on install/trigger thread");render(e,l,r,64);}
        e.allNotesOff();render(e,l,r,100);finished.store(true,std::memory_order_release);collector.join();e.collectRetiredSamples();check(e.retainedSampleAssets()==1,"concurrent worker collector safely retires superseded voice assets");
    }

    {
        auto original=std::make_unique<Engine>(),foreign=std::make_unique<Engine>();original->prepare(8000,64);foreign->prepare(8000,64);Parameters p;p.engine=11;p.attack=.001f;p.decay=.001f;p.sustain=1;p.release=.005f;p.sampleLoop=true;p.volume=1;original->setParameters(p);original->setOutputGainImmediately(1);
        auto asset=original->registerPreparedSample(Engine::prepareSample(std::vector<float>(800,.2f),{},8000));check(!foreign->installPreparedSample(asset),"foreign engine rejects prepared install");foreign->discardPreparedSample(asset);check(original->collectRetiredSamples()==0,"foreign discard cannot release producer pin");check(original->installPreparedSample(asset),"valid owner still installs after foreign attempts");original->noteOn(60,1);render(*original,l,r,100);
        auto pending=original->registerPreparedSample(Engine::prepareSample(std::vector<float>(800,.4f),{},8000));auto moved=std::make_unique<Engine>(std::move(*original));check(original->sampleLeft().empty(),"moved-from engine clears current sample pointer");original->prepare(8000,64);original->loadSample({.9f},{},8000);check(!original->installPreparedSample(pending),"reused moved-from engine has fresh registry identity");original->discardPreparedSample(pending);check(moved->collectRetiredSamples()==0,"moved-from reuse cannot retire destination voice or pending asset");render(*moved,l,r,100);check(std::abs(l.back()-.2f)<1e-6,"moved voice keeps original immutable asset after source reuse");check(moved->installPreparedSample(pending),"pending ticket owner identity survives engine move");moved->allNotesOff();render(*moved,l,r,100);check(moved->collectRetiredSamples()==1,"moved sample references retire exactly once");
        *foreign=std::move(*moved);moved->prepare(8000,64);moved->loadSample({-.1f},{},8000);foreign->noteOn(60,1);render(*foreign,l,r,100);check(std::abs(l.back()-.4f)<1e-6,"move assignment and moved-from reuse retain destination asset ownership");
    }
    {
        auto a=std::make_unique<Engine>(),reference=std::make_unique<Engine>();a->prepare(8000,64);reference->prepare(8000,64);Parameters p;p.engine=11;p.sampleLoop=true;p.attack=.001f;p.decay=.001f;p.sustain=1;p.release=.005f;p.volume=1;std::vector<float> wave(8000);for(size_t i=0;i<wave.size();++i)wave[i]=.3f*std::sin(i*6.283185307179586*220/8000);a->loadSample(wave,{},8000);reference->loadSample(wave,{},8000);a->setParameters(p);reference->setParameters(p);a->setOutputGainImmediately(1);reference->setOutputGainImmediately(1);a->noteOn(60,1);reference->noteOn(60,1);render(*a,l,r,1000);std::vector<float> expectedL,expectedR;render(*reference,expectedL,expectedR,1000);
        auto replacement=a->registerPreparedSample(Engine::prepareSample(std::vector<float>(8000,.25f),{},16000));a->installPreparedSample(replacement);p.sampleRootNote=72;a->setParameters(p);render(*a,l,r,1000);render(*reference,expectedL,expectedR,1000);check(l==expectedL&&r==expectedR,"sample turnover preserves old voice rate/root/bounds, not just its audio memory");
    }
    {
        TapeAudio live;live.assign(8200,.2f,-.3f);live.prepareWrite(20000);const auto pinned=live;
        const auto allocations=watchedAllocations.load(),frees=watchedDeallocations.load();watchAllocations=true;
        auto snapshot=live;const bool wrote=live.tryWrite(18000,.7f,-.8f);watchAllocations=false;
        check(wrote&&watchedAllocations.load()==allocations&&watchedDeallocations.load()==frees,"paged snapshot and sparse stereo write allocate/free nothing");
        check(pinned.size()==8200&&snapshot.size()==8200&&pinned.read(0,18000)==0&&live.read(0,18000)==.7f&&live.read(1,18000)==-.8f,"pinned tape generations retain length and stereo samples");
        check(live.read(0,15000)==0,"long seek creates sparse zero space");live.resize(5000);live.resize(19000);check(live.read(0,18000)==0&&live.read(1,7000)==0,"truncated tape cannot resurrect after regrowth");
        TapeAudio copy;copy=live;TapeAudio moved;moved=std::move(copy);live=TapeAudio{};check(moved.size()==19000&&moved.read(0,20)==.2f,"pool lifetime survives copy/move assignment and last source destruction");
        const auto generation=moved;std::atomic<bool> done{false},valid{true};std::thread reader([&]{while(!done.load(std::memory_order_acquire))for(size_t f=0;f<5000;f+=113)if(generation.read(0,f)!=.2f||generation.read(1,f)!=-.3f)valid=false;});
        moved.prepareWrite(19000);for(size_t f=0;f<5000;++f)moved.write(f,.9f,.8f);done.store(true,std::memory_order_release);reader.join();check(valid.load(),"concurrent snapshot reader sees immutable audio during a live overwrite");
    }
    {
        auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,64);Parameters p;p.monitorInput=true;p.volume=1;e.setParameters(p);e.setOutputGainImmediately(1);e.setRecording(true);e.seek(300);
        l.assign(64,.2f);r.assign(64,-.1f);const auto allocations=watchedAllocations.load(),frees=watchedDeallocations.load();watchAllocations=true;e.process(l.data(),r.data(),l.data(),r.data(),64);watchAllocations=false;
        check(watchedAllocations.load()==allocations&&watchedDeallocations.load()==frees,"six-minute sparse recording gap does not allocate or free in callback");check(e.tracks()[0].audio.size()==2400064&&e.tracks()[0].audio.read(0,1200000)==0&&e.tracks()[0].audio.read(0,2400000)==.2f,"live recording writes only the distant page and preserves logical zeros");
        e.tracks()[0].audio=TapeAudio{};l.assign(64,.2f);r.assign(64,-.1f);e.process(l.data(),r.data(),l.data(),r.data(),64);check(!e.recording()&&e.tapeWriteFailures()==1&&energy(l)>.001f,"missing recording reserve stops recording while monitored audio continues");
    }
    {
        TapeAudio live;live.assign(1,.1f,-.1f);std::vector<TapeAudio> pins;pins.reserve(128);bool exhausted=false;
        for(int i=0;i<128;++i){pins.push_back(live);const auto before=live.readStereo(0);const auto allocations=watchedAllocations.load(),frees=watchedDeallocations.load();watchAllocations=true;const bool written=live.tryWrite(0,(i+1)*.01f,-(i+1)*.01f);watchAllocations=false;check(watchedAllocations.load()==allocations&&watchedDeallocations.load()==frees,"pinned generation pressure never falls back to heap allocation/free");if(!written){exhausted=true;check(live.readStereo(0)==before,"pool exhaustion preserves both channels of the last committed sample");break;}}
        check(exhausted,"bounded recording pool reports exhaustion under pinned generation pressure");pins.clear();live.collect(10000);live.prepareWrite(1);check(live.tryWrite(0,.7f,-.7f),"control reclamation restores recording capacity after saved generations retire");
    }
    {auto owner=std::make_unique<Engine>();auto& e=*owner;e.prepare(8000,128);e.tracks()[0].audio.assign(800,.2f,-.1f);const auto base=e.captureTapeEdit();auto edit=Engine::prepareTapeEdit(base,TapeEditAction::lift,1,.02,.04);check(e.tracks()[0].audio.read(0,200)==.2f,"preparing tape edit leaves live audio untouched");check(e.commitTapeEdit(base,std::move(edit))&&e.tracks()[0].audio.read(0,200)==0,"prepared edit commits atomically");e.undo();check(e.tracks()[0].audio.read(0,200)==.2f,"prepared edit preserves undo");auto staleBase=e.captureTapeEdit();auto stale=Engine::prepareTapeEdit(staleBase,TapeEditAction::clear,1,0,.1);e.tracks()[0].audio.prepareWrite(800);e.tracks()[0].audio.write(0,.9f,.8f);check(!e.commitTapeEdit(staleBase,std::move(stale))&&e.tracks()[0].audio.read(0,0)==.9f,"stale prepared edit cannot erase newer overdub");const auto cap=field::tape::TapePool::memoryLimit();field::tape::TapePool::setMemoryLimit(field::tape::TapePool::memoryUsed());auto clearBase=e.captureTapeEdit();auto cleared=Engine::prepareTapeEdit(clearBase,TapeEditAction::clear,1,0,.1);check(e.commitTapeEdit(clearBase,std::move(cleared))&&e.tracks()[0].audio.empty(),"clear remains possible when global budget is full");field::tape::TapePool::setMemoryLimit(cap);e.discardTapeHistory();auto retired=e.takeRetiredTape();}
    std::cout<<"PASS: tuning, release, recording, stereo, overdub, undo/redo, lift/drop, splice, resampling, forward/reverse loops, export, mute/solo, sampler slices, engines/effects, sequencer\n";
}
