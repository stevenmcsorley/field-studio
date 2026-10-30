#include "Engine.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace field {
namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double tau = pi * 2.0;
// Software physical keyboard starts at F3. Hardware's absolute drum octave is
// not verified; this mapping keeps its leftmost displayed key on pad one.
constexpr int drumBaseNote = 53;
int drumPadIndex(int note) { return ((note-drumBaseNote)%24+24)%24; }
float clamp(float x, float a = 0, float b = 1) { return std::clamp(std::isfinite(x) ? x : a, a, b); }
double wrap(double x) { return x - std::floor(x); }
float noise(uint32_t& state) {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return static_cast<float>((state & 0xffffffu) / 8388607.5 - 1.0);
}
float blep(double phase, double dt) {
    if (phase < dt) { double t = phase / dt; return static_cast<float>(t + t - t*t - 1.0); }
    if (phase > 1.0 - dt) { double t = (phase - 1.0) / dt; return static_cast<float>(t*t + t + t + 1.0); }
    return 0;
}
float saw(double phase, double dt) { return static_cast<float>(phase*2.0-1.0) - blep(phase,dt); }
float pulse(double phase, double dt, double width) {
    return (phase < width ? 1.0f : -1.0f) + blep(phase,dt) - blep(wrap(phase-width),dt);
}
float sine(double phase) { return static_cast<float>(std::sin(tau*phase)); }
float interpolate(const std::vector<float>& data, double at) {
    if (data.empty() || at < 0 || at >= static_cast<double>(data.size())) return 0;
    const auto index = static_cast<size_t>(at);
    const auto next = std::min(index+1,data.size()-1);
    const float frac = static_cast<float>(at-static_cast<double>(index));
    return data[index] + frac * (data[next]-data[index]);
}
size_t snapshotBytes(const Track& t) { return (t.left.size()+t.right.size())*sizeof(float); }
}

Engine::Engine() {
    resetDrumPads();resetDboxPads();steps.fill(-1);stepVelocity.fill(.8f);
    constexpr int initial[16]={60,-1,67,-1,63,-1,70,67,60,-1,75,-1,70,67,63,-1};
    constexpr float accents[16]={.9f,.7f,.7f,.7f,.8f,.7f,.7f,.7f,.9f,.7f,.7f,.7f,.8f,.7f,.7f,.7f};
    std::copy(std::begin(initial),std::end(initial),steps.begin());std::copy(std::begin(accents),std::end(accents),stepVelocity.begin());
    for(size_t b=0;b<fingerBanks.size();++b){auto& bank=fingerBanks[b];bank.note.fill(-1);bank.velocity.fill(.8f);for(int i=0;i<16;++i)if(initial[i]>=0)bank.note[i]=std::clamp(initial[i]+static_cast<int>(b)%5,0,127);}
    for(size_t i=0;i<sketch.size();++i){const float x=static_cast<float>(i)/static_cast<float>(sketch.size()-1);sketch[i]={x,.5f-.25f*sine(x)};}
    for(int i=0;i<sketchCount;++i){const float x=static_cast<float>(i)/(sketchCount-1);sketch[static_cast<size_t>(i)]={x,.5f-.25f*sine(x)};}
}
const DrumPad& Engine::drumPad(int index) const { return pads[static_cast<size_t>(std::clamp(index,0,23))]; }
void Engine::setDrumPad(int index,const DrumPad& source) {
    if(index<0 || index>=24)return;
    auto p=source;p.start=clamp(p.start);p.end=clamp(p.end,p.start,1);
    p.tune=clamp(p.tune,-24,24);p.level=clamp(p.level,0,2);p.pan=clamp(p.pan,-1,1);
    p.mode=std::clamp(p.mode,0,2);p.chokeGroup=std::clamp(p.chokeGroup,0,4);
    pads[static_cast<size_t>(index)]=p;
}
void Engine::resetDrumPads() {
    for(size_t i=0;i<pads.size();++i){pads[i]=DrumPad{};pads[i].start=static_cast<float>(i)/24;pads[i].end=static_cast<float>(i+1)/24;}
}
const DBoxPad& Engine::dboxPad(int index) const { return dboxPads[static_cast<size_t>(std::clamp(index,0,23))]; }
void Engine::setDboxPad(int index,const DBoxPad& source) {
    if(index<0 || index>=24)return;
    auto p=source;p.mix=clamp(p.mix);p.wave1=clamp(p.wave1);p.envelope1=clamp(p.envelope1,.005f,4);p.crossMod=clamp(p.crossMod);
    p.frequency=clamp(p.frequency);p.wave2=clamp(p.wave2);p.envelope2=clamp(p.envelope2,.005f,4);p.cutoff=clamp(p.cutoff);
    p.attack=clamp(p.attack,.0002f,1);p.gain=clamp(p.gain,0,2);p.release=clamp(p.release,.005f,4);p.smooth=clamp(p.smooth);
    dboxPads[static_cast<size_t>(index)]=p;
}
void Engine::resetDboxPads(){
    for(size_t i=0;i<dboxPads.size();++i){
        auto p=DBoxPad{};const int kind=static_cast<int>(i)%12;
        if(kind==0 || kind==1 || kind==7){p.frequency=.13f+kind*.005f;p.mix=.08f;p.wave2=.1f;p.envelope1=.24f;p.envelope2=.035f;p.crossMod=.04f;p.cutoff=.4f;}
        else if(kind==2 || kind==3 || kind==5){p.frequency=.38f;p.wave1=.15f;p.mix=.65f;p.wave2=1;p.envelope1=.12f;p.envelope2=.11f;p.crossMod=.15f;p.cutoff=.8f;}
        else{p.frequency=.68f+kind*.008f;p.mix=.8f;p.wave1=.8f;p.wave2=.95f;p.envelope1=.025f;p.envelope2=kind==6?.25f:.055f;p.cutoff=.95f;p.crossMod=.35f;}
        if(i>=12){p.frequency=clamp(p.frequency+.055f);p.envelope1*=1.25f;}
        dboxPads[i]=p;
    }
}
void Engine::setSelectedPad(int index){padSelected=std::clamp(index,0,23);}
void Engine::prepare(double rate, int /* maxBlock */) {
    clearSequenceGates();
    const double nextSr = std::clamp(std::isfinite(rate)?rate:44100.0, 8000.0, 192000.0);
    if (nextSr != sr) {
        const double ratio = nextSr / sr;
        for (auto& t : tape) {
            if (!t.left.empty()) {
                const size_t n = static_cast<size_t>(std::min(360.0 * nextSr, t.left.size()*ratio));
                std::vector<float> l(n), r(n);
                for (size_t j=0;j<n;++j) { l[j]=interpolate(t.left,j/ratio); r[j]=interpolate(t.right,j/ratio); }
                t.left.swap(l); t.right.swap(r);
            }
        }
        position *= ratio; loopBegin *= ratio; loopFinish *= ratio;
        undos.clear(); redos.clear();
    }
    sr = nextSr;
    delayL.assign(static_cast<size_t>(sr*2.1)+1,0);
    delayR.assign(delayL.size(),0);
    delayWrite=0; fxLowL=fxLowR=fxBandL=fxBandR=wetLowL=wetLowR=0;
    phaserL.fill(0); phaserR.fill(0);
    for (auto& v : voices) v.active = false;keyCount=0;
    isRecording=false; isPlaying=false;countInRemaining=0;countInElapsed=0;reverseHeld=false;brakeHeld=false;rollHeld=false;
    seqPhase=0; seqStep=-1;seqPositionPending=-1; seqLastNote=-1;
    peakValue=0;
    for(size_t i=0;i<vocoder.size();++i){
        const double frequency=std::min(sr*.42,100.0*std::pow(1.5,static_cast<double>(i)));
        const double omega=tau*frequency/sr,alpha=std::sin(omega)/(2*3.5),a0=1+alpha;
        vocoder[i]={};vocoder[i].b0=static_cast<float>(alpha/a0);vocoder[i].b2=-vocoder[i].b0;
        vocoder[i].a1=static_cast<float>(-2*std::cos(omega)/a0);vocoder[i].a2=static_cast<float>((1-alpha)/a0);
    }
}
void Engine::setParameters(const Parameters& p) {
    const bool modeChanged=params.voiceMode!=p.voiceMode;const bool sequenceWasOn=params.sequenceOn;
    const bool drumChanged=params.drumMode!=p.drumMode;
    const bool sequencerChanged=params.sequencer!=p.sequencer;
    params=p;
    params.engine=std::clamp(params.engine,0,14);params.voiceMode=std::clamp(params.voiceMode,0,3);params.glideTime=clamp(params.glideTime,0,4);params.patchGain=clamp(params.patchGain,0,1);
    for (float& m : params.macro) m=clamp(m);
    for (float& m : params.effectMacro) m=clamp(m);
    for (float& m : params.sequencerMacro) m=clamp(m);
    params.lfoTarget=std::clamp(params.lfoTarget,0,19);
    params.lfoWaveform=std::clamp(params.lfoWaveform,0,5);
    params.lfoDivision=std::clamp(params.lfoDivision,0,11);
    params.elementSource=std::clamp(params.elementSource,0,3);
    params.elementManual=clamp(params.elementManual);
    params.envelopeCurve=std::clamp(params.envelopeCurve,0,1);
    for(size_t i=0;i<4;++i){params.midiLfoTargets[i]=std::clamp(params.midiLfoTargets[i],0,19);params.midiLfoDepth[i]=clamp(params.midiLfoDepth[i],-1,1);}

    params.seqSwing=clamp(params.seqSwing,0,.5f);params.seqGate=clamp(params.seqGate,.05f,1);
    params.attack=clamp(params.attack,.001f,10); params.decay=clamp(params.decay,.005f,10);
    params.sustain=clamp(params.sustain); params.release=clamp(params.release,.005f,20);
    params.effect=std::clamp(params.effect,0,10); params.effectMix=clamp(params.effectMix);
    params.volume=clamp(params.volume,0,2.f); params.tempo=clamp(params.tempo,30,300);
    params.lfoRate=clamp(params.lfoRate,.01f,30); params.lfoDepth=clamp(params.lfoDepth);
    params.tapeSpeed=clamp(params.tapeSpeed,-4,4);
    params.metronomeLevel=clamp(params.metronomeLevel);
    params.tapeRecordLevel=clamp(params.tapeRecordLevel,0,2);params.tapeRecordPan=clamp(params.tapeRecordPan,-1,1);
    params.sampleRootNote=clamp(params.sampleRootNote,0,127);params.sampleStart=clamp(params.sampleStart);params.sampleEnd=clamp(params.sampleEnd,params.sampleStart,1);
    params.sampleLoopIn=clamp(params.sampleLoopIn,params.sampleStart,params.sampleEnd);
    params.sampleLoopOut=clamp(params.sampleLoopOut,params.sampleLoopIn,params.sampleEnd);
    if (sequenceWasOn!=params.sequenceOn || drumChanged || sequencerChanged || modeChanged) {
        for (auto& v : voices) if (v.active) { v.gate=false; v.stage=3; v.releaseStart=v.env;v.envelopeProgress=0; }
        keyCount=0;heldCount=0; seqStep=-1;seqPositionPending=-1; seqPhase=0; seqLastNote=-1;clearSequenceGates();
    }
}
int Engine::activeVoiceCount()const {return static_cast<int>(std::count_if(voices.begin(),voices.end(),[](const Voice& v){return v.active;}));}
float Engine::currentVoicePitch()const {for(const auto& v:voices)if(v.active)return v.pitch;return -1;}
void Engine::trigger(int note,float velocity){
    if(!params.drumMode&&params.engine!=14&&(params.voiceMode==1||params.voiceMode==2)){
        auto& old=voices[0];const bool held=old.active&&old.gate;const float pitch=old.active?old.pitch:(float)note;
        if(params.voiceMode==2&&held){old.note=std::clamp(note,0,127);old.velocity=clamp(velocity);old.detune=0;if(params.glideTime<=0)old.pitch=(float)old.note;return;}
        for(auto& v:voices)v.active=false;triggerVoice(note,velocity);if(params.glideTime>0&&held)voices[0].pitch=pitch;return;
    }
    if(!params.drumMode&&params.engine!=14&&params.voiceMode==3){for(float detune:{-.12f,-.04f,.04f,.12f})triggerVoice(note,velocity*.4f,detune);return;}
    triggerVoice(note,velocity);
}
void Engine::triggerVoice(int note,float velocity,float detune) {
    note=std::clamp(note,0,127);
    const int padIndex=drumPadIndex(note);
    if(params.drumMode && params.engine==11){
        padSelected=padIndex;const auto& pad=pads[static_cast<size_t>(padIndex)];
        for(auto& existing:voices)if(existing.active && existing.drum && existing.engine==11 &&
            (existing.pad==padIndex || (pad.chokeGroup>0 && existing.chokeGroup==pad.chokeGroup))){
            existing.gate=false;existing.stage=3;existing.releaseStart=existing.env;existing.envelopeProgress=0;
            // Three-millisecond microfade avoids discontinuities when a pad
            // retriggers or another pad in its choke group takes over.
            existing.forcedRelease=.003f;
        }
    }
    auto* voice=&voices[0];
    for (auto& v : voices) {
        if (!v.active) { voice=&v; break; }
        if (v.env<voice->env || (v.env==voice->env && v.age>voice->age)) voice=&v;
    }
    Voice& v=*voice;
    v.active=true; v.gate=true; v.note=note;v.pitch=note+detune;v.detune=detune;v.stringPrevious=0; v.velocity=clamp(velocity);
    v.stage=0; v.env=0;v.envelopeProgress=0;if(params.lfoRetrigger){lfoPhase=0;lfoClock=0;}v.forcedRelease=0; v.phase=0; v.phase2=.173; v.age=0; v.samplePos=0;
    v.lp=v.hp=0; v.vocL1.fill(0);v.vocL2.fill(0);v.vocR1.fill(0);v.vocR2.fill(0);v.engine=params.engine; v.drum=params.drumMode || params.engine==14;
    v.noise=0x9e3779b9u ^ (static_cast<uint32_t>(note)*1664525u + ++randomState);
    if (!v.noise) v.noise=1;
    const double frequency=440.0*std::pow(2.0,(note-69)/12.0);
    v.stringLength=std::clamp(static_cast<int>(sr/frequency),2,4096);
    v.stringPos=0;
    if (v.engine==6) {
        for (int j=0;j<4096;++j) v.string[static_cast<size_t>(j)]=noise(v.noise)*.8f;
    }
    if(v.engine==14){
        padSelected=padIndex;v.pad=padIndex;v.dbox=dboxPads[static_cast<size_t>(padIndex)];
        v.dboxLowL=v.dboxBandL=v.dboxLowR=v.dboxBandR=v.dboxPrevious=0;
        const double cutoff=std::min(sr*.4,80.0+v.dbox.cutoff*v.dbox.cutoff*16000.0);
        const double omega=tau*cutoff/sr,alpha=std::sin(omega)/(2*(.55+v.dbox.crossMod*3)),a0=1+alpha;
        v.dboxFilter={static_cast<float>((1-std::cos(omega))*.5/a0),static_cast<float>((1-std::cos(omega))/a0),static_cast<float>((1-std::cos(omega))*.5/a0),static_cast<float>(-2*std::cos(omega)/a0),static_cast<float>((1-alpha)/a0)};
    }
    if (v.engine==11 && !sampleL.empty()) {
        v.sampleTune=0;v.sampleLevel=1;v.samplePan=0;v.pad=-1;v.chokeGroup=0;
        if(v.drum){
            const auto& pad=pads[static_cast<size_t>(padIndex)];
            v.pad=padIndex;v.chokeGroup=pad.chokeGroup;
            v.sampleBegin=sampleL.size()*pad.start;v.sampleFinish=sampleL.size()*pad.end;
            v.sampleLoopBegin=v.sampleBegin;v.sampleLoopFinish=v.sampleFinish;
            v.sampleReverse=pad.reverse;v.sampleLooped=pad.mode==2;v.sampleOneShot=pad.mode==0;
            v.sampleTune=pad.tune;v.sampleLevel=pad.level;v.samplePan=pad.pan;
        }else{
            v.sampleBegin=sampleL.size()*params.sampleStart;v.sampleFinish=sampleL.size()*params.sampleEnd;
            v.sampleLoopBegin=sampleL.size()*params.sampleLoopIn;v.sampleLoopFinish=sampleL.size()*params.sampleLoopOut;
            v.sampleReverse=params.sampleReverse;v.sampleLooped=params.sampleLoop;v.sampleOneShot=params.sampleOneShot;
        }
        if(v.sampleFinish-v.sampleBegin<1){v.active=false;return;}
        v.samplePos=v.sampleReverse?std::max(v.sampleBegin,v.sampleFinish-1):v.sampleBegin;
    }
}
void Engine::noteOn(int note,float velocity) {
    if(params.lfoRetrigger&&velocity>0){lfoPhase=0;lfoClock=0;}
    if (velocity<=0) { noteOff(note); return; }
    note=std::clamp(note,0,127);
    if(!params.sequenceOn&&!params.drumMode&&params.engine!=14){for(int i=0;i<keyCount;++i)if(keyStack[(size_t)i]==note){for(int j=i;j<keyCount-1;++j)keyStack[(size_t)j]=keyStack[(size_t)j+1];--keyCount;break;}if(keyCount<128)keyStack[(size_t)keyCount++]=note;keyVelocity[(size_t)note]=clamp(velocity);}
    if (params.sequenceOn && params.sequencer>0) {
        for (int j=0;j<heldCount;++j) if (heldNotes[j]==note) { heldVelocities[j]=velocity; return; }
        if(params.sequencer==3)fingerSelected=((note-drumBaseNote)%16+16)%16;
        if (heldCount<static_cast<int>(heldNotes.size())) {
            heldNotes[heldCount]=note; heldVelocities[heldCount]=clamp(velocity); ++heldCount;
        }
        if(params.sequencer==4){trigger(note,velocity);if(seqStep<0)seqStep=0;}
        else if(params.sequencer==7){addBall(note,velocity);}
        else if (seqStep<0) { seqPhase=0; tickSequence(); }
    } else trigger(note,velocity);
    heldLfo=clamp(velocity);
}
void Engine::releaseNote(int note) {
    for (auto& v : voices) if (v.active && v.note==note && v.gate) {
        if (v.drum && v.engine!=11) continue;
        if (v.engine==11 && v.sampleOneShot) continue;
        v.gate=false; v.stage=3; v.releaseStart=v.env;v.envelopeProgress=0;
    }
}
void Engine::noteOff(int note) {
    if(!params.sequenceOn&&!params.drumMode&&params.engine!=14){for(int i=0;i<keyCount;++i)if(keyStack[(size_t)i]==note){for(int j=i;j<keyCount-1;++j)keyStack[(size_t)j]=keyStack[(size_t)j+1];--keyCount;break;}if((params.voiceMode==1||params.voiceMode==2)&&voices[0].active&&voices[0].note==note&&keyCount>0){int fallback=keyStack[(size_t)keyCount-1];trigger(fallback,keyVelocity[(size_t)fallback]);return;}}

    if (params.sequenceOn && (params.sequencer==4 || params.seqHold)) return;
    if(params.sequencer==7)for(auto& ball:balls)if(ball.note==note)ball.active=false;
    releaseNote(note);
    for (int j=0;j<heldCount;++j) if (heldNotes[j]==note) {
        for (int k=j;k<heldCount-1;++k) { heldNotes[k]=heldNotes[k+1]; heldVelocities[k]=heldVelocities[k+1]; }
        --heldCount; break;
    }
    if(heldCount==0){clearSequenceGates();seqLastNote=-1;}
}
void Engine::allNotesOff() {
    keyCount=0;
    for (auto& v : voices) { v.gate=false; v.stage=3; v.releaseStart=v.env;v.envelopeProgress=0;v.forcedRelease=.005f; }
    heldCount=0; seqLastNote=-1; seqStep=-1;seqPositionPending=-1; seqPhase=0;
    for(auto& ball:balls)ball.active=false;
    clearSequenceGates();
    ballClock=0;
}
void Engine::beginMidiBlock(){
    midiEventSize=0;midiOverflow=false;midiFrame=0;midiFrameCursor=0;
    if(pendingMidiOverflow){midiEventBuffer[0]={-1,0,false,0};midiEventSize=1;midiOverflow=true;}
    else for(size_t i=0;i<pendingMidiSize;++i)midiEventBuffer[midiEventSize++]=pendingMidiEvents[i];
    pendingMidiSize=0;pendingMidiOverflow=false;
}
void Engine::emitSequenceMidi(int note,float velocity,bool on){
    auto& events=midiProcessing?midiEventBuffer:pendingMidiEvents;
    auto& size=midiProcessing?midiEventSize:pendingMidiSize;
    auto& overflow=midiProcessing?midiOverflow:pendingMidiOverflow;
    if(overflow)return;
    const int frame=midiProcessing?midiFrame:0;
    if(size==events.size()){size=1;events[0]={-1,0,false,frame};overflow=true;return;}
    events[size++]={note,on?clamp(velocity):0,on,frame};
}
void Engine::clearSequenceGates(){for(auto& gate:noteGates){if(gate.note>=0){emitSequenceMidi(gate.note,0,false);releaseNote(gate.note);}gate={};}}
void Engine::gatedTrigger(int note,float velocity,double seconds){
    for(auto& gate:noteGates)if(gate.note==note){emitSequenceMidi(note,0,false);gate={};releaseNote(note);}
    trigger(note,velocity);emitSequenceMidi(note,velocity,true);
    auto* slot=&noteGates[0];for(auto& gate:noteGates)if(gate.note<0){slot=&gate;break;}
    if(slot->note>=0){emitSequenceMidi(slot->note,0,false);releaseNote(slot->note);}
    slot->note=note;slot->remaining=std::max(1.0,seconds*sr);seqLastNote=note;
}
void Engine::tickGates(){
    for(auto& gate:noteGates)if(gate.note>=0){gate.remaining-=1;if(gate.remaining<=0){emitSequenceMidi(gate.note,0,false);releaseNote(gate.note);gate={};}}
}
void Engine::tickSequence() {
    const int clockLength=params.sequencer==3?32:seqLength;
    seqStep=seqPositionPending>=0?seqPositionPending%clockLength:(seqStep+1)%clockLength;seqPositionPending=-1;
    if (heldCount<=0) { seqLastNote=-1; return; }
    int note=heldNotes[0];float velocity=heldVelocities[0];
    const double stepSeconds=60.0/(params.tempo*4.0)*(1+(seqStep%2==0?params.seqSwing:-params.seqSwing));
    const double gate=stepSeconds*params.seqGate;
    switch(params.sequencer){
        case 1:{
            int index=seqStep%heldCount;
            if(params.sequencerMacro[0]>.75f)index=static_cast<int>((noise(randomState)+1)*.499f*heldCount)%heldCount;
            else if(params.sequencerMacro[0]>.5f && heldCount>1){int n=seqStep%(2*heldCount-2);index=n<heldCount?n:2*heldCount-2-n;}
            else if(params.sequencerMacro[0]>.25f)index=heldCount-1-index;
            note=heldNotes[index];velocity=heldVelocities[index];break;
        }
        case 2:
        case 5:
            if(steps[seqStep]<0){seqLastNote=-1;return;}
            note=std::clamp(steps[seqStep]+heldNotes[0]-60,0,127);velocity=stepVelocity[seqStep]*heldVelocities[0];break;
        case 3:
            for(int i=0;i<heldCount;++i){
                const int bankIndex=((heldNotes[i]-drumBaseNote)%16+16)%16;
                const auto& bank=fingerBanks[static_cast<size_t>(bankIndex)];const int step=seqStep%bank.length;
                if(bank.note[step]>=0)gatedTrigger(bank.note[step],bank.velocity[step]*heldVelocities[i],gate);
            }
            return;
        case 4:return;
        case 6:{
            const float x=clockLength>1?static_cast<float>(seqStep)/(clockLength-1):0;
            bool found=false;float y=.5f;
            for(int i=0;i<sketchCount-1;++i){const auto& a=sketch[static_cast<size_t>(i)];const auto& b=sketch[static_cast<size_t>(i+1)];
                if(x>=std::min(a.x,b.x) && x<=std::max(a.x,b.x)){
                    const float length=b.x-a.x;const float t=std::abs(length)<.000001f?0:clamp((x-a.x)/length);
                    y=a.y+(b.y-a.y)*t;found=true;break;
                }
            }
            if(!found){seqLastNote=-1;return;}
            note=std::clamp(heldNotes[0]+static_cast<int>(std::round((.5f-y)*24)),0,127);
            velocity=heldVelocities[0]*(.5f+.5f*params.sequencerMacro[3]);break;
        }
        case 7:return;
        default:return;
    }
    gatedTrigger(note,velocity,gate);
}
void Engine::addBall(int note,float velocity){
    for(size_t i=0;i<balls.size();++i)if(balls[i].active && balls[i].note==note){ballVelocity[i]=clamp(velocity);return;}
    size_t index=0;for(size_t i=0;i<balls.size();++i)if(!balls[i].active){index=i;break;}
    balls[index]={.5f+noise(randomState)*.12f,.35f+noise(randomState)*.03f,noise(randomState)*.8f,-.7f-clamp(velocity)*.8f,note,true};
    ballVelocity[index]=clamp(velocity);ballCooldown[index]=0;
}
int Engine::tombolaSides() const {return std::clamp(3+static_cast<int>(params.sequencerMacro[2]*5.999f),3,8);}
void Engine::tickTombola(){
    constexpr float dt=1.0f/120,radius=.018f;
    const float rotation=(params.sequencerMacro[0]-.5f)*4;
    ballAngle=std::remainder(ballAngle+rotation*dt,tau);
    const int sides=tombolaSides();const float boundary=.44f*static_cast<float>(std::cos(pi/sides))-radius;
    const float gravity=.5f+12*params.sequencerMacro[1],restitution=.2f+.78f*params.sequencerMacro[3];
    const double gate=60.0/(params.tempo*4.0)*params.seqGate;
    for(size_t i=0;i<balls.size();++i){auto& ball=balls[i];if(!ball.active)continue;
        ballCooldown[i]=std::max(0.0f,ballCooldown[i]-dt);ball.vy+=gravity*dt;ball.vx*=.9995f;ball.vy*=.9995f;
        ball.x+=ball.vx*dt;ball.y+=ball.vy*dt;
        float impact=0;
        for(int pass=0;pass<2;++pass)for(int wall=0;wall<sides;++wall){
            const double angle=ballAngle+wall*tau/sides;const float nx=static_cast<float>(std::cos(angle)),ny=static_cast<float>(std::sin(angle));
            const float distance=(ball.x-.5f)*nx+(ball.y-.5f)*ny-boundary;
            if(distance>0){
                ball.x-=distance*nx;ball.y-=distance*ny;
                const float wallVx=-rotation*(ball.y-.5f),wallVy=rotation*(ball.x-.5f);
                const float speed=(ball.vx-wallVx)*nx+(ball.vy-wallVy)*ny;
                if(speed>0){ball.vx-=(1+restitution)*speed*nx;ball.vy-=(1+restitution)*speed*ny;impact=std::max(impact,speed);}
            }
        }
        ball.x=clamp(ball.x,radius,1-radius);ball.y=clamp(ball.y,radius,1-radius);
        ball.vx=clamp(ball.vx,-8,8);ball.vy=clamp(ball.vy,-8,8);
        if(impact>.07f && ballCooldown[i]<=0){gatedTrigger(ball.note,ballVelocity[i]*clamp(.2f+impact*.35f),gate);++ballTriggers;ballCooldown[i]=.035f;}
    }
    // Equal-mass ball collisions transfer momentum; container impacts trigger
    // notes. Fixed120Hz stepping keeps simulation independent of block size.
    for(size_t i=0;i<balls.size();++i)if(balls[i].active)for(size_t j=i+1;j<balls.size();++j)if(balls[j].active){
        auto& a=balls[i];auto& b=balls[j];const float dx=b.x-a.x,dy=b.y-a.y,d2=dx*dx+dy*dy;
        if(d2>0.000001f && d2<4*radius*radius){
            const float distance=std::sqrt(d2),nx=dx/distance,ny=dy/distance;
            const float shift=(2*radius-distance)*.5f;a.x-=shift*nx;a.y-=shift*ny;b.x+=shift*nx;b.y+=shift*ny;
            const float speed=(b.vx-a.vx)*nx+(b.vy-a.vy)*ny;if(speed<0){const float impulse=-(1+restitution)*speed*.5f;a.vx-=impulse*nx;a.vy-=impulse*ny;b.vx+=impulse*nx;b.vy+=impulse*ny;}
        }
    }
    // Pair separation can push a neighbour into a wall. Reproject the final
    // snapshot so every published centre stays inside the same container.
    for(auto& ball:balls)if(ball.active){
        for(int pass=0;pass<3;++pass)for(int wall=0;wall<sides;++wall){const double angle=ballAngle+wall*tau/sides;const float nx=static_cast<float>(std::cos(angle)),ny=static_cast<float>(std::sin(angle));const float distance=(ball.x-.5f)*nx+(ball.y-.5f)*ny-boundary;if(distance>0){ball.x-=distance*nx;ball.y-=distance*ny;}}
        ball.x=clamp(ball.x,radius,1-radius);ball.y=clamp(ball.y,radius,1-radius);
    }
}
std::pair<float,float> Engine::renderDbox(Voice& v,float /*modulation*/){
    const auto& p=v.dbox;
    const float pitch=voiceRoute(0,v.velocity)*.7f;
    const float mix=clamp(p.mix+(voiceRoute(1,v.velocity)*.5f));
    const float wave1=clamp(p.wave1+(voiceRoute(2,v.velocity)*.5f));
    const float cross=clamp(p.crossMod+(voiceRoute(4,v.velocity)*.5f));
    const float wave2=clamp(p.wave2+voiceRoute(3,v.velocity)*.5f);
    const double frequency=std::min(sr*.3,25.0*std::pow(2.0,p.frequency*8.0+(pitch+pitchBend)/12.0));
    const double dt=frequency/sr,dt2=std::min(.42,dt*(1.0+std::floor(wave2*7.0)));
    const float n1=noise(v.noise),n2=noise(v.noise);
    auto shape=[&](double phase,double increment,float wave,float n){
        const float shapes[5]{sine(phase),static_cast<float>(1-4*std::abs(phase-.5)),saw(phase,increment),pulse(phase,increment,.5),n};
        const float at=clamp(wave)*4;const int a=std::min(3,static_cast<int>(at));return shapes[a]+(shapes[a+1]-shapes[a])*(at-a);
    };
    const double age=v.age;
    const float env1=static_cast<float>(std::exp(-age/p.envelope1));
    const float env2=static_cast<float>(std::exp(-age/p.envelope2));
    const float osc2=shape(v.phase2,dt2,wave2,n2);
    const float osc1=shape(wrap(v.phase+osc2*cross*1.5),dt,wave1,n1);
    float signal=osc1*env1*(1-mix)+osc2*env2*mix;
    // Smooth regulates transient slew before the resonant low-pass stage.
    const float smoothing=1-std::exp(-1.0f/(static_cast<float>(sr)*(.00001f+p.smooth*.0004f)));
    v.dboxPrevious+=smoothing*(signal-v.dboxPrevious);signal=v.dboxPrevious;
    const auto& f=v.dboxFilter;const float filtered=f[0]*signal+v.dboxLowL;
    v.dboxLowL=f[1]*signal-f[3]*filtered+v.dboxBandL;v.dboxBandL=f[2]*signal-f[4]*filtered;
    const float attack=static_cast<float>(std::min(1.0,age/p.attack));
    const double tail=std::max(0.0,age-std::max(p.envelope1,p.envelope2)*4.0);
    const float transient=attack*static_cast<float>(std::exp(-tail/p.release));
    if(!v.gate || v.stage==3){
        const float duration=v.forcedRelease>0?v.forcedRelease:p.release;
        v.env=std::max(0.0f,v.env-v.releaseStart/(duration*static_cast<float>(sr)));
        if(v.env<=.000001f)v.active=false;
    }else v.env=transient;
    const float gain=p.gain*v.velocity*.65f*v.env*(std::max(0.0f,1+voiceRoute(5,v.velocity)));
    const float pan=clamp(voiceRoute(6,v.velocity),-1,1);
    const float out=std::tanh(filtered*1.2f)*gain;
    v.phase=wrap(v.phase+dt);v.phase2=wrap(v.phase2+dt2);v.age+=1.0/sr;
    if(env1+env2<.00001f || age>20)v.active=false;
    return {out*(pan>0?1-pan:1),out*(pan<0?1+pan:1)};
}
std::pair<float,float> Engine::renderVoice(Voice& v,float modulation,float inL,float inR) {
    if (!v.active) return {0,0};
    if(v.engine==14)return renderDbox(v,modulation);
    const float target=v.note+v.detune;if(!v.drum&&(params.voiceMode==1||params.voiceMode==2)&&params.glideTime>0)v.pitch+=(target-v.pitch)*(1-std::exp(-1.f/(params.glideTime*(float)sr)));else v.pitch=target;
    const float attack=std::clamp(params.attack*std::pow(4.f,voiceRoute(8,v.velocity)),.001f,40.f);
    const float decay=std::clamp(params.decay*std::pow(4.f,voiceRoute(9,v.velocity)),.001f,40.f);
    const float sustain=clamp(params.sustain+voiceRoute(10,v.velocity)*.5f);
    const float release=v.forcedRelease>0?v.forcedRelease:std::clamp(params.release*std::pow(4.f,voiceRoute(11,v.velocity)),.001f,40.f);
    if(params.envelopeCurve==1){
        const float duration=v.stage==0?attack:v.stage==1?decay:release;
        if(v.stage==2)v.env=sustain;
        else {
            v.envelopeProgress=std::min(1.f,v.envelopeProgress+1.f/(duration*static_cast<float>(sr)));
            const float shaped=(1.f-std::exp(-5.f*v.envelopeProgress))/(1.f-std::exp(-5.f));
            v.env=v.stage==0?shaped:v.stage==1?1.f-(1.f-sustain)*shaped:v.releaseStart*(1.f-shaped);
            if(v.envelopeProgress>=1){v.envelopeProgress=0;if(v.stage==3){v.active=false;return {0,0};}++v.stage;}
        }
    }else{
        const float a=1.f/(attack*static_cast<float>(sr)), d=(1.f-sustain)/(decay*static_cast<float>(sr));
        if(v.stage==0){v.env+=a;if(v.env>=1){v.env=1;v.stage=1;}}
        else if(v.stage==1){v.env-=d;if(v.env<=sustain){v.env=sustain;v.stage=2;}}
        else if(v.stage==2)v.env=sustain;
        else {v.env-=std::max(.0000001f,v.releaseStart/(release*static_cast<float>(sr)));if(v.env<=0){v.active=false;return {0,0};}}
    }
    const float pitchMod=voiceRoute(0,v.velocity)*.7f;
    const double freq=std::clamp(440.0*std::pow(2.0,(v.pitch-69+pitchBend+pitchMod)/12.0),8.0,sr*.42);
    const double dt=freq/sr;
    const float x=clamp(params.macro[0]+(voiceRoute(1,v.velocity)*.5f));
    const float y=clamp(params.macro[1]+(voiceRoute(2,v.velocity)*.5f));
    const float z=clamp(params.macro[2]+(voiceRoute(3,v.velocity)*.5f));
    const float w=clamp(params.macro[3]+(voiceRoute(4,v.velocity)*.5f));
    float l=0,r=0;
    if (v.drum && v.engine!=11) {
        const int kind=drumPadIndex(v.note)%12;
        const float age=static_cast<float>(v.age);
        const float n=noise(v.noise);
        if (kind==0 || kind==1 || kind==7) {
            const float base=35.0f+x*100.0f;
            v.phase=wrap(v.phase+(base+180.0*std::exp(-age*(25+y*90)))/sr);
            l=(sine(v.phase)+n*.06f*std::exp(-age*180))*std::exp(-age*(3+w*18));
        } else if (kind==2 || kind==3 || kind==5) {
            v.phase=wrap(v.phase+(100+x*200)/sr);
            l=(n*(.6f+.35f*z)+sine(v.phase)*.4f)*std::exp(-age*(7+w*25));
        } else {
            v.lp+=.35f*(n-v.lp);
            l=(n-v.lp)*std::exp(-age*(kind==6?4.0f:18.0f+w*60));
        }
        r=l;
        if (age>3 || (std::abs(l)<.00001f && age>1)) v.active=false;
    } else {
        const double p=v.phase, q=v.phase2;
        const float n=noise(v.noise);
        switch (v.engine) {
            case 0: { // Voltage: band-limited saw/pulse/triangle blend.
                const float sa=saw(p,dt), pu=pulse(p,dt,.08+.84*y);
                const float tri=static_cast<float>(1.0-4.0*std::abs(p-.5));
                l=x<.5f ? sa*(1-x*2)+pu*x*2 : pu*(2-x*2)+tri*(x*2-1);
                l=l*.8f+sine(q)*w*.2f; r=l; break;
            }
            case 1: { // FM: carrier/modulator ratios and feedback-like phase warp.
                const double ratio=1+std::round(x*11);
                l=static_cast<float>(std::sin(tau*p+(y*y*14.0)*std::sin(tau*q*ratio+z*std::sin(tau*p))));
                r=static_cast<float>(std::sin(tau*p+(y*y*14.0)*std::sin(tau*q*ratio+w*.12))); break;
            }
            case 2: { // Cluster: spread detuned oscillators.
                l=(saw(p,dt)+saw(wrap(q),dt)+saw(wrap(p+q*.003*(1+x)),dt))*.33f;
                r=(saw(p,dt)+saw(wrap(p-(q-p)*(1+y)),dt)+sine(q))*.33f; break;
            }
            case 3: { // Digital: quantised additive harmonics.
                const int h=2+static_cast<int>(x*14);
                float out=sine(p)+sine(p*h)*y+sine(p*(h+1))*z*.5f;
                const float levels=4.0f+std::pow(2.0f,w*10);
                l=r=std::round(out*levels)/levels*.5f; break;
            }
            case 4: { // Dr Wave: stepped shape through a wave-shaping function.
                const double shape=p+std::sin(tau*p*(1+std::round(y*5)))*x*.18;
                l=std::tanh(static_cast<float>(std::sin(tau*shape))*(1+z*9));
                r=std::tanh(static_cast<float>(std::sin(tau*(shape+w*.015)))*(1+z*9)); break;
            }
            case 5: { // Dimension: stereo unison chord oscillator.
                const double fifth=std::pow(2.0,7.0/12.0);
                l=(sine(p)+saw(q,dt)*x+sine(p*fifth)*y*.5f)*.55f;
                r=(sine(p)+saw(wrap(p-(q-p)),dt)*x+sine(p*fifth+w*.03)*y*.5f)*.55f; break;
            }
            case 6: { // Fractional-delay waveguide supports glide and pitch bend.
                const double delay=std::clamp(sr/freq,2.0,4094.0);double read=v.stringPos-delay;if(read<0)read+=4096;
                int j=(int)read,k=(j+1)%4096;float fraction=(float)(read-j);l=v.string[(size_t)j]+(v.string[(size_t)k]-v.string[(size_t)j])*fraction;
                v.string[(size_t)v.stringPos]=(l*(.35f+x*.3f)+v.stringPrevious*(.65f-x*.3f))*(.99f+z*.0097f);v.stringPrevious=l;v.stringPos=(v.stringPos+1)%4096;r=l;break;
            }
            case 7: { // Pulse: pulse-width and hard-sync-like overtones.
                l=pulse(p,dt,.05+.9*x);
                r=pulse(wrap(p+y*.015),dt,.05+.9*x);
                l=l*(1-z*.5f)+sine(p*(1+std::round(w*7)))*z*.5f; break;
            }
            case 8: { // Phase: phase distortion.
                const double pivot=.05+.9*x;
                const double warped=p<pivot ? p*.5/pivot : .5+(p-pivot)*.5/(1-pivot);
                l=sine(warped+y*std::sin(tau*p)*.4); r=sine(warped+z*std::sin(tau*p)*.4); break;
            }
            case 9: { // DNA: deterministic noise-modulated tonal texture.
                const double cell=std::floor(p*(4+std::round(x*60)));
                const float hash=static_cast<float>(std::sin(cell*12.9898+v.note*78.233)*43758.5453);
                const float granular=(hash-std::floor(hash))*2-1;
                l=sine(p)*(1-y)+granular*y+n*z*.15f;
                r=l*(1-w*.2f)+sine(q)*w*.2f; break;
            }
            case 10: { // DSynth: independently shaped dual oscillators.
                l=saw(p,dt)*(1-x)+pulse(p,dt,.1+.8*y)*x;
                r=sine(q)*(1-z)+saw(q,dt)*z;
                const float mid=(l+r)*.5f; l=mid*(1-w)+l*w; r=mid*(1-w)+r*w; break;
            }
            case 11: {
                if(sampleL.empty() || v.sampleFinish<=v.sampleBegin){v.active=false;return {0,0};}
                l=interpolate(sampleL,v.samplePos);r=interpolate(sampleR,v.samplePos);
                l*=v.sampleLevel*(v.samplePan>0?1-v.samplePan:1);
                r*=v.sampleLevel*(v.samplePan<0?1+v.samplePan:1);
                const double tune=v.sampleTune+(v.drum?0:v.pitch-params.sampleRootNote)+pitchBend+pitchMod;
                const double inc=sampleSr/sr*std::pow(2.0,tune/12.0)*(v.sampleReverse?-1:1);
                v.samplePos+=inc;
                const bool loop=v.sampleLooped && v.gate && v.sampleLoopFinish-v.sampleLoopBegin>=1;
                if(loop){
                    const double span=v.sampleLoopFinish-v.sampleLoopBegin;
                    if(!v.sampleReverse && v.samplePos>=v.sampleLoopFinish)
                        v.samplePos=v.sampleLoopBegin+std::fmod(v.samplePos-v.sampleLoopFinish,span);
                    else if(v.sampleReverse && v.samplePos<v.sampleLoopBegin)
                        v.samplePos=v.sampleLoopFinish-1-std::fmod(v.sampleLoopBegin-v.samplePos-1,span);
                }
                if(v.samplePos>=v.sampleFinish || v.samplePos<v.sampleBegin)v.active=false;
                break;
            }
            case 12: { // Original twelve-band analysis/synthesis vocoder.
                const float carrierL=saw(p,dt)*(1-y)+pulse(p,dt,.1+x*.8)*y;
                const float carrierR=saw(wrap(p+w*.008),dt)*(1-y)+pulse(wrap(p+w*.008),dt,.1+x*.8)*y;
                for(size_t band=0;band<vocoder.size();++band){
                    const auto& b=vocoder[band];
                    const float cl=b.b0*carrierL+v.vocL1[band];
                    v.vocL1[band]=-b.a1*cl+v.vocL2[band];v.vocL2[band]=b.b2*carrierL-b.a2*cl;
                    const float cr=b.b0*carrierR+v.vocR1[band];
                    v.vocR1[band]=-b.a1*cr+v.vocR2[band];v.vocR2[band]=b.b2*carrierR-b.a2*cr;
                    const float envelope=vocoder[band].envelope;
                    l+=cl*envelope*18;r+=cr*envelope*18;
                }
                break;
            }
            case 13: l=std::tanh(inL*(1+x*14))*(.4f+y*.6f); r=std::tanh(inR*(1+x*14))*(.4f+y*.6f); break;
            default: break;
        }
        v.phase=wrap(v.phase+dt);
        v.phase2=wrap(v.phase2+dt*(1.0+(x-.5)*.025));
        const float filter=clamp(z+(voiceRoute(7,v.velocity)*.5f));
        const float cutoff=std::min(static_cast<float>(sr*.4),100.0f+filter*filter*18000.0f);
        const float coefficient=1.0f-std::exp(-static_cast<float>(tau)*cutoff/static_cast<float>(sr));
        if(v.engine!=11 || (voiceRoute(7,v.velocity)!=0)){
            v.lp+=coefficient*(l-v.lp);
            v.hp+=coefficient*(r-v.hp);
            l=v.lp; r=v.hp;
        }
    }
    v.age+=1.0/sr;
    const float volumeMod=std::max(0.0f,1+voiceRoute(5,v.velocity));
    const float pan=clamp(voiceRoute(6,v.velocity),-1,1);
    const float gain=v.env*v.velocity*(v.engine==11?1.f:(v.drum?.42f:.19f))*volumeMod;
    return {l*gain*(pan>0?1-pan:1),r*gain*(pan<0?1+pan:1)};
}
std::pair<float,float> Engine::effect(float l,float r) {
    if (delayL.empty()) return {l,r};
    auto read=[&](const std::vector<float>& d,double samples) {
        double index=static_cast<double>(delayWrite)-std::clamp(samples,1.0,static_cast<double>(d.size()-2));
        if(index<0)index+=d.size();
        const size_t i=static_cast<size_t>(index), n=(i+1)%d.size();
        const float fraction=static_cast<float>(index-i);
        return d[i]+fraction*(d[n]-d[i]);
    };
    const float x=clamp(params.effectMacro[0]+modulationRoutes[12]*.5f), y=clamp(params.effectMacro[1]+modulationRoutes[13]*.5f), z=clamp(params.effectMacro[2]+modulationRoutes[14]*.5f), w=clamp(params.effectMacro[3]+modulationRoutes[15]*.5f);
    float wetL=l,wetR=r,feedbackL=0,feedbackR=0;
    const float tapL=read(delayL,(.04+x*.71)*sr), tapR=read(delayR,(.057+x*.713)*sr);
    fxPhase=wrap(fxPhase+(0.05+y*1.8)/sr);
    switch(params.effect) {
        case 1: wetL=tapL; wetR=tapR; feedbackL=tapR*(.15f+y*.65f); feedbackR=tapL*(.15f+y*.65f); break;
        case 2: { // Short dispersive feedback taps: spring-inspired, not a model of TE's engine.
            wetL=read(delayL,sr*.0311)+read(delayR,sr*.0477)*.7f+read(delayL,sr*.0713)*.5f;
            wetR=read(delayR,sr*.0371)+read(delayL,sr*.0537)*.7f+read(delayR,sr*.0793)*.5f;
            wetL*=.5f;wetR*=.5f;feedbackL=wetR*(.45f+y*.4f);feedbackR=wetL*(.45f+y*.4f);break;
        }
        case 3: wetL=(tapL+read(delayR,sr*(.1+x*.4)))*.5f; wetR=(tapR+read(delayL,sr*(.17+x*.4)))*.5f;feedbackL=wetR*.6f;feedbackR=wetL*.6f;break;
        case 4: { // State-variable-like filter.
            const float coeff=1-std::exp(-static_cast<float>(tau)*(100+std::pow(x,2)*12000)/static_cast<float>(sr));
            fxLowL+=coeff*(l-fxLowL);fxLowR+=coeff*(r-fxLowR);
            wetL=fxLowL*(1-y)+(l-fxLowL)*y;wetR=fxLowR*(1-y)+(r-fxLowR)*y; break;
        }
        case 5: {
            const float low=1-std::exp(-static_cast<float>(tau)*3500/static_cast<float>(sr));
            const float high=1-std::exp(-static_cast<float>(tau)*420/static_cast<float>(sr));
            fxLowL+=low*(l-fxLowL);fxLowR+=low*(r-fxLowR);
            fxBandL+=high*(fxLowL-fxBandL);fxBandR+=high*(fxLowR-fxBandR);
            wetL=std::tanh((fxLowL-fxBandL)*(2+x*7));wetR=std::tanh((fxLowR-fxBandR)*(2+x*7));break;
        }
        case 6: wetL=std::tanh(l*(1+x*15))*.8f;wetR=std::tanh(r*(1+x*15))*.8f;break;
        case 7: {
            double ring=wrap(fxPhase*(4+std::round(x*500)));
            wetL=(l+tapL*y*.4f)*sine(ring);wetR=(r+tapR*y*.4f)*sine(ring+.25);
            feedbackL=wetL*.3f;feedbackR=wetR*.3f;break;
        }
        case 8: { const float levels=4+std::pow(2.0f,(1-x)*10); wetL=std::round((l+tapL*y*.4f)*levels)/levels;wetR=std::round((r+tapR*y*.4f)*levels)/levels;feedbackL=wetL*.35f;feedbackR=wetR*.35f;break; }
        case 9: { const double movement=(.03+.45*(1-fxPhase))*sr;wetL=read(delayL,movement);wetR=read(delayR,movement+sr*.013);feedbackL=wetR*.5f;feedbackR=wetL*.5f;break; }
        case 10: { // Four-stage all-pass phaser.
            const float a=.1f+.8f*(.5f+.5f*sine(fxPhase));float dl=l,dr=r;
            for(size_t j=0;j<4;++j){float nl=-a*dl+phaserL[j];phaserL[j]=dl+a*nl;dl=nl;float nr=-a*dr+phaserR[j];phaserR[j]=dr+a*nr;dr=nr;}
            wetL=dl;wetR=dr;break;
        }
        default: break;
    }
    delayL[delayWrite]=std::tanh(l+feedbackL);delayR[delayWrite]=std::tanh(r+feedbackR);
    delayWrite=(delayWrite+1)%delayL.size();
    // Consistent wet-tone and stereo-width controls complement each effect's
    // two core controls. These are original software controls, not recovered
    // proprietary engine parameter meanings.
    const float tone=1-std::exp(-static_cast<float>(tau)*std::min(static_cast<float>(sr*.42),200+z*z*18000)/static_cast<float>(sr));
    wetLowL+=tone*(wetL-wetLowL);wetLowR+=tone*(wetR-wetLowR);
    const float mid=(wetLowL+wetLowR)*.5f,side=(wetLowL-wetLowR)*w;
    wetL=mid+side;wetR=mid-side;
    const float mix=params.effect==0?0:clamp(params.effectMix+modulationRoutes[16]*.5f);
    return {l*(1-mix)+wetL*mix,r*(1-mix)+wetR*mix};
}
void Engine::process(float* left,float* right,const float* inputLeft,const float* inputRight,int frames,float* const* stems) {
    if(!left || !right || frames<=0)return;
    const bool anySolo=std::any_of(tape.begin(),tape.end(),[](const Track& t){return t.solo;});
    midiProcessing=true;midiFrame=midiFrameCursor;
    if(pendingMidiOverflow){midiEventBuffer[0]={-1,0,false,midiFrame};midiEventSize=1;midiOverflow=true;}
    else for(size_t i=0;i<pendingMidiSize;++i)emitSequenceMidi(pendingMidiEvents[i].note,pendingMidiEvents[i].velocity,pendingMidiEvents[i].on);
    pendingMidiSize=0;pendingMidiOverflow=false;
    for(int i=0;i<frames;++i){
        midiFrame=static_cast<int>(std::min<int64_t>(2147483647,static_cast<int64_t>(midiFrameCursor)+i));
        if(stems)for(int channel=0;channel<8;++channel)if(stems[channel])stems[channel][i]=0;
        const float rawL=inputLeft?inputLeft[i]:0,rawR=inputRight?inputRight[i]:rawL;
        const float inL=std::isfinite(rawL)?rawL:0,inR=std::isfinite(rawR)?rawR:0;
        tickGates();
        if(params.sequenceOn && params.sequencer==7){ballClock+=120.0/sr;while(ballClock>=1){ballClock-=1;tickTombola();}}
        else if(params.sequenceOn && params.sequencer>0 && params.sequencer!=4 && heldCount>0){
            seqPhase+=params.tempo/(60.0*sr)*4.0;
            const double length=1+(seqStep%2==0?params.seqSwing:-params.seqSwing);
            if(seqPhase+1e-10>=length){seqPhase=std::max(0.0,seqPhase-length);tickSequence();}
        }
        const float envTarget=(std::abs(inL)+std::abs(inR))*.5f;
        inputEnv+=(envTarget>inputEnv?.01f:.0005f)*(envTarget-inputEnv);
        constexpr double beats[]{16,8,4,2,1,.75,.5,.375,.25,.1875,.125,.0625};
        const double rate=params.lfoSync?params.tempo/(60.*beats[params.lfoDivision]):params.lfoRate;
        lfoPhase=wrap(lfoPhase+rate/sr);
        lfoClock+=rate/sr;
        if(lfoClock>=1){lfoClock-=1;randomLfo=noise(randomState);}
        float modulation=0;
        switch(params.lfo){case 0:modulation=randomLfo;break;case 1:{float source=inputEnv;if(params.elementSource==1){source=0;for(const auto& voice:voices)if(voice.active)source=std::max(source,voice.env);}else if(params.elementSource==2)source=previousOutputEnvelope;else if(params.elementSource==3)source=params.elementManual;modulation=clamp(source)*2-1;break;}case 2:modulation=modWheel*2-1;break;case 3:switch(params.lfoWaveform){case 0:modulation=sine(lfoPhase);break;case 1:modulation=1.f-4.f*std::abs(static_cast<float>(lfoPhase)-.5f);break;case 2:modulation=static_cast<float>(lfoPhase)*2-1;break;case 3:modulation=lfoPhase<.5?1.f:-1.f;break;case 4:modulation=2.f*(std::exp(4.f*static_cast<float>(lfoPhase))-1.f)/(std::exp(4.f)-1.f)-1.f;break;case 5:modulation=lfoPhase<.1?1.f:-1.f;break;}break;case 4:modulation=params.macro[3]*2-1;break;case 5:modulation=heldLfo*2-1;break;default:break;}
        modulation*=params.lfoDepth;
        modulationRoutes.fill(0);
        bool midiMatrix=false;
        if(params.lfo==2)for(size_t c=0;c<4;++c)if(params.midiLfoDepth[c]!=0){midiMatrix=true;auto& route=modulationRoutes[params.midiLfoTargets[c]];route=clamp(route+(midiControllers[c]*2-1)*params.midiLfoDepth[c],-1,1);}
        if(!midiMatrix)modulationRoutes[params.lfoTarget]=modulation;
        if(params.engine==12){
            const float modulator=(inL+inR)*.5f;
            for(auto& b:vocoder){
                const float analysed=b.b0*modulator+b.z1;
                b.z1=-b.a1*analysed+b.z2;b.z2=b.b2*modulator-b.a2*analysed;
                const float target=std::abs(analysed);
                b.envelope+=(target>b.envelope?.035f:.0015f)*(target-b.envelope);
            }
        }
        float synthL=0,synthR=0;
        for(auto& v:voices)if(v.active){const auto s=renderVoice(v,modulation,inL,inR);synthL+=s.first;synthR+=s.second;}
        // Amp/input processing works without a held key, as an input engine.
        if(params.engine==13 && !params.drumMode){synthL=std::tanh(inL*(1+params.macro[0]*14))*.65f;synthR=std::tanh(inR*(1+params.macro[0]*14))*.65f;}
        if(params.monitorInput && params.engine!=13 && params.engine!=12){synthL+=inL;synthR+=inR;}
        const auto processed=effect(synthL*params.patchGain*std::max(0.f,1+modulationRoutes[17]),synthR*params.patchGain*std::max(0.f,1+modulationRoutes[17]));synthL=processed.first;synthR=processed.second;
        float mixL=synthL,mixR=synthR;
        const bool counting=countInRemaining>0&&isPlaying&&isRecording;
        if(counting){const double phase=countInElapsed-std::floor(countInElapsed);if(phase<.08){const double seconds=phase*60./params.tempo;const float click=static_cast<float>(std::sin(seconds*1900*tau)*std::exp(-seconds*80))*.13f*params.metronomeLevel;mixL+=click;mixR+=click;}const double step=params.tempo/(60.*sr);countInElapsed+=step;countInRemaining=std::max(0.,countInRemaining-step);if(countInRemaining<1e-9)countInRemaining=0;}
        if(isPlaying&&!counting){
            if(rollHeld){const double length=std::min(rollLength,static_cast<double>(maxFrames())-rollBegin);if(length>0&&(position<rollBegin||position>=rollBegin+length))position=rollBegin+std::fmod(std::fmod(position-rollBegin,length)+length,length);}
            if(!rollHeld && params.loop && loopFinish>loopBegin+1){
                const double length=loopFinish-loopBegin;
                if(position>=loopFinish || position<loopBegin){position=loopBegin+std::fmod(std::fmod(position-loopBegin,length)+length,length);}
            }
            if(position<0 || position>=static_cast<double>(maxFrames())){isPlaying=false;isRecording=false;position=std::clamp(position,0.0,static_cast<double>(maxFrames()-1));}
            if(isPlaying){
                for(size_t track=0;track<tape.size();++track){
                    const auto& t=tape[track];if(t.mute || (anySolo&&!t.solo))continue;
                    const float panL=t.pan>0?1-t.pan:1,panR=t.pan<0?1+t.pan:1;
                    const float trackL=brakeHeld?0:interpolate(t.left,position)*t.level*panL;
                    const float trackR=brakeHeld?0:interpolate(t.right,position)*t.level*panR;
                    mixL+=trackL;mixR+=trackR;
                    if(stems){if(stems[track*2])stems[track*2][i]=trackL;if(stems[track*2+1])stems[track*2+1][i]=trackR;}
                }
                if(isRecording&&!brakeHeld){
                    auto& t=tape[static_cast<size_t>(selected)];const auto at=static_cast<size_t>(position);
                    // Capacity was reserved in setRecording(). Never allocate in
                    // the audio callback, even after malformed restored state.
                    if(at<t.left.capacity() && at<t.right.capacity()){
                        if(at>=t.left.size()){t.left.resize(at+1,0);t.right.resize(at+1,0);}
                        t.left[at]=std::clamp(t.left[at]+synthL*params.tapeRecordLevel*(params.tapeRecordPan>0?1-params.tapeRecordPan:1),-4.0f,4.0f);
                        t.right[at]=std::clamp(t.right[at]+synthR*params.tapeRecordLevel*(params.tapeRecordPan<0?1+params.tapeRecordPan:1),-4.0f,4.0f);
                    } else {isRecording=false;}
                }
                if(params.metronome&&!brakeHeld){const double beat=position/sr*params.tempo/60.0;const double phase=beat-std::floor(beat);if(phase<.045){float click=static_cast<float>(std::sin(phase*sr/params.tempo*60*1900*tau))*static_cast<float>(std::exp(-phase*130))*.08f*params.metronomeLevel;mixL+=click;mixR+=click;}}
                if(!brakeHeld)position+=params.tapeSpeed*std::pow(2.f,modulationRoutes[18])*(reverseHeld?-1.:1.);
            }
        }
        switch(params.tapeStyle){
            case 1:mixL=std::tanh(mixL*1.25f)*.8f;mixR=std::tanh(mixR*1.25f)*.8f;break;
            case 2:mixL=std::tanh(mixL*1.7f)*.65f;mixR=std::tanh(mixR*1.7f)*.65f;break;
            case 3:mixL=std::round(mixL*8192)/8192;mixR=std::round(mixR*8192)/8192;break;
            default:break;
        }
        smoothedVolume+=.001f*(clamp(params.volume*(1+modulationRoutes[19]),0,2.f)-smoothedVolume);
        left[i]=std::clamp(mixL*smoothedVolume,-1.0f,1.0f);right[i]=std::clamp(mixR*smoothedVolume,-1.0f,1.0f);
        if(!std::isfinite(left[i]))left[i]=0;
        if(!std::isfinite(right[i]))right[i]=0;
        previousOutputEnvelope=std::max(std::abs(left[i]),std::abs(right[i]));
        peakValue=std::max(previousOutputEnvelope,peakValue*.9997f);
    }
    midiProcessing=false;midiFrameCursor=static_cast<int>(std::min<int64_t>(2147483647,static_cast<int64_t>(midiFrameCursor)+frames));
}
void Engine::loadSample(const std::vector<float>& l,const std::vector<float>& r,double rate) {
    allNotesOff();
    // Preserve up to two minutes of stereo sample data, matching the drum
    // sampler capacity. The wrapper can enforce melodic capture limits.
    const double safeRate=std::clamp(std::isfinite(rate)?rate:44100.0,8000.0,192000.0);
    const auto limit=static_cast<size_t>(safeRate*120.0);
    sampleL.assign(l.begin(),l.begin()+static_cast<std::ptrdiff_t>(std::min(l.size(),limit)));
    sampleR.resize(sampleL.size());
    for(size_t i=0;i<sampleL.size();++i){sampleL[i]=clamp(sampleL[i],-4,4);sampleR[i]=r.empty()?sampleL[i]:clamp(r[std::min(i,r.size()-1)],-4,4);}
    sampleSr=safeRate;
}
void Engine::selectTrack(int i){selected=std::clamp(i,0,3);if(isRecording){tape[selected].left.reserve(maxFrames());tape[selected].right.reserve(maxFrames());}}
void Engine::setTrackLevel(int i,float v){if(i>=0&&i<4)tape[i].level=clamp(v,0,2);}
void Engine::setTrackPan(int i,float v){if(i>=0&&i<4)tape[i].pan=clamp(v,-1,1);}
void Engine::setTrackMute(int i,bool v){if(i>=0&&i<4)tape[i].mute=v;}
void Engine::setTrackSolo(int i,bool v){if(i>=0&&i<4)tape[i].solo=v;}
float Engine::trackLevel(int i)const{return tape[std::clamp(i,0,3)].level;}
float Engine::trackPan(int i)const{return tape[std::clamp(i,0,3)].pan;}
bool Engine::trackMute(int i)const{return tape[std::clamp(i,0,3)].mute;}
bool Engine::trackSolo(int i)const{return tape[std::clamp(i,0,3)].solo;}
void Engine::setPlaying(bool v){isPlaying=v;if(!v){clearSequenceGates();isRecording=false;countInRemaining=0;reverseHeld=false;brakeHeld=false;rollHeld=false;}}
Engine::Snapshot Engine::capture(unsigned mask)const{Snapshot s;s.track=selected;s.head=position;s.mask=mask?mask&15u:1u<<selected;for(int i=0;i<4;++i)if(s.mask&(1u<<i))s.audio[i]=tape[i];return s;}
void Engine::trimHistory(){
    constexpr size_t limit=256u*1024u*1024u;
    auto bytes=[](const std::deque<Snapshot>& q){size_t total=0;for(const auto& s:q)for(int i=0;i<4;++i)if(s.mask&(1u<<i))total+=snapshotBytes(s.audio[i]);return total;};
    while(undos.size()>7)undos.pop_front();
    while(redos.size()>7)redos.pop_front();
    while(undos.size()>1 && bytes(undos)+bytes(redos)>limit)undos.pop_front();
    while(redos.size()>1 && bytes(undos)+bytes(redos)>limit)redos.pop_front();
}
void Engine::snapshot(unsigned mask){undos.push_back(capture(mask));redos.clear();trimHistory();}
void Engine::setRecording(bool v){
    if(v&&!isRecording){snapshot();tape[selected].left.reserve(maxFrames());tape[selected].right.reserve(maxFrames());isPlaying=true;countInRemaining=countInSetting;countInElapsed=0;}
    isRecording=v;if(!v)countInRemaining=0;
}
void Engine::rewind(){position=params.loop?loopBegin:0;seqPhase=0;seqStep=-1;seqPositionPending=-1;}
void Engine::seek(double seconds){position=std::clamp(std::isfinite(seconds)?seconds:0.0,0.0,359.999)*sr;}
void Engine::setLoop(double start,double end){start=std::isfinite(start)?start:0;end=std::isfinite(end)?end:2;loopBegin=std::clamp(start,0.0,359.999)*sr;loopFinish=std::clamp(end,loopBegin/sr+1.0/sr,360.0)*sr;}
double Engine::recordedSeconds()const{size_t n=0;for(const auto&t:tape)n=std::max(n,t.left.size());return n/sr;}
void Engine::clearTrack(){snapshot();auto& t=tape[selected];t.left.clear();t.right.clear();t.cuts.clear();}
void Engine::copyTracks(unsigned mask,double start,double end){
    mask=mask?mask&15u:1u<<selected;
    clipboardMask=0;clipboardFrames=0;for(auto& t:clipboard)t=Track{};
    start=std::clamp(std::isfinite(start)?start:0.,0.,360.);end=std::clamp(std::isfinite(end)?end:0.,0.,360.);
    const size_t a=static_cast<size_t>(start*sr),b=static_cast<size_t>(end*sr);if(a>=b)return;
    size_t last=0;for(int i=0;i<4;++i)if(mask&(1u<<i))last=std::max(last,tape[i].left.size());
    if(a>=std::min(b,last))return;clipboardFrames=std::min(b,last)-a;clipboardMask=mask;
    for(int i=0;i<4;++i)if(mask&(1u<<i)){
        const auto& t=tape[i];auto& c=clipboard[i];c.left.assign(clipboardFrames,0);c.right.assign(clipboardFrames,0);
        if(a<t.left.size()){const size_t n=std::min(clipboardFrames,t.left.size()-a);std::copy_n(t.left.begin()+a,n,c.left.begin());std::copy_n(t.right.begin()+a,n,c.right.begin());}
        c.level=t.level;c.pan=t.pan;c.mute=t.mute;c.solo=t.solo;
        for(double cut:t.cuts)if(cut>=start&&cut<=end)c.cuts.push_back(cut-start);
    }
}
void Engine::lift(double start,double end){liftTracks(1u<<selected,start,end);}
void Engine::liftTracks(unsigned mask,double start,double end){
    if(!std::isfinite(start)||!std::isfinite(end))return;
    copyTracks(mask,start,end);if(!clipboardMask)return;snapshot(clipboardMask);
    const size_t a=static_cast<size_t>(std::clamp(start,0.,360.)*sr);
    for(int i=0;i<4;++i)if(clipboardMask&(1u<<i)){auto& t=tape[i];const size_t b=std::min(t.left.size(),a+clipboardFrames);if(a>=b)continue;std::fill(t.left.begin()+a,t.left.begin()+b,0);std::fill(t.right.begin()+a,t.right.begin()+b,0);t.cuts.erase(std::remove_if(t.cuts.begin(),t.cuts.end(),[&](double c){return c>=start&&c<=end;}),t.cuts.end());}
    position=static_cast<double>(a);
}
void Engine::drop(){dropTracks();}
void Engine::dropTracks(int destination){
    if(!clipboardMask)return;if(destination<0)destination=selected;destination=std::clamp(destination,0,3);
    int first=0;while(!(clipboardMask&(1u<<first)))++first;
    unsigned mask=0;for(int i=first;i<4;++i)if((clipboardMask&(1u<<i))&&destination+i-first<4)mask|=1u<<(destination+i-first);
    snapshot(mask);const size_t a=static_cast<size_t>(std::clamp(position,0.,static_cast<double>(maxFrames()-1)));
    for(int i=first;i<4;++i)if((clipboardMask&(1u<<i))&&destination+i-first<4){auto& t=tape[destination+i-first];const auto& c=clipboard[i];const size_t n=std::min(c.left.size(),maxFrames()-a);
        t.left.resize(std::max(t.left.size(),a+n),0);t.right.resize(t.left.size(),0);std::copy_n(c.left.begin(),n,t.left.begin()+a);std::copy_n(c.right.begin(),n,t.right.begin()+a);
        t.cuts.erase(std::remove_if(t.cuts.begin(),t.cuts.end(),[&](double cut){return cut>=a/sr&&cut<=(a+n)/sr;}),t.cuts.end());
        for(double cut:c.cuts)if(cut<=n/sr)t.cuts.push_back(cut+a/sr);t.cuts.push_back(a/sr);t.cuts.push_back((a+n)/sr);std::sort(t.cuts.begin(),t.cuts.end());t.cuts.erase(std::unique(t.cuts.begin(),t.cuts.end()),t.cuts.end());
    }
}
void Engine::mergeClipboard(int destination){
    if(!clipboardMask)return;destination=destination<0?selected:std::clamp(destination,0,3);snapshot(1u<<destination);writeClipboardMix(destination);
}
void Engine::writeClipboardMix(int destination){
    auto& t=tape[destination];const size_t a=static_cast<size_t>(std::clamp(position,0.,static_cast<double>(maxFrames()-1))),n=std::min(clipboardFrames,maxFrames()-a);
    t.left.resize(std::max(t.left.size(),a+n),0);t.right.resize(t.left.size(),0);
    bool solo=false;for(int i=0;i<4;++i)if(clipboardMask&(1u<<i))solo|=clipboard[i].solo;
    for(size_t f=0;f<n;++f){float l=0,r=0;for(int i=0;i<4;++i)if(clipboardMask&(1u<<i)){const auto& c=clipboard[i];if(c.mute||(solo&&!c.solo)||f>=c.left.size())continue;l+=c.left[f]*c.level*(c.pan>0?1-c.pan:1);r+=c.right[f]*c.level*(c.pan<0?1+c.pan:1);}t.left[a+f]=std::clamp(l,-4.f,4.f);t.right[a+f]=std::clamp(r,-4.f,4.f);}
    t.cuts.push_back(a/sr);t.cuts.push_back((a+n)/sr);std::sort(t.cuts.begin(),t.cuts.end());
}
void Engine::joinTracks(unsigned mask,double start,double end){
    mask=mask?mask&15u:1u<<selected;
    if(!std::isfinite(start)||!std::isfinite(end)||start>=end||!(mask&15u))return;snapshot(mask&15u);
    for(int i=0;i<4;++i)if(mask&(1u<<i)){auto& c=tape[i].cuts;c.erase(std::remove_if(c.begin(),c.end(),[&](double cut){return cut>start&&cut<end;}),c.end());}
}
void Engine::slideTracks(unsigned mask,double start,double end,double delta){
    mask=mask?mask&15u:1u<<selected;
    if(!std::isfinite(start)||!std::isfinite(end)||!std::isfinite(delta)||start>=end||delta==0||!(mask&15u))return;
    start=std::clamp(start,0.,360.);end=std::clamp(end,0.,360.);delta=std::clamp(delta,-start,360.-end);if(delta==0)return;
    const size_t a=static_cast<size_t>(start*sr),b=static_cast<size_t>(end*sr),at=static_cast<size_t>((start+delta)*sr);if(a>=b)return;snapshot(mask&15u);
    for(int i=0;i<4;++i)if(mask&(1u<<i)){auto& t=tape[i];if(a>=t.left.size())continue;const size_t finish=std::min(b,t.left.size()),n=finish-a;
        std::vector<float> l(t.left.begin()+a,t.left.begin()+finish),r(t.right.begin()+a,t.right.begin()+finish);std::vector<double> cuts;for(double cut:t.cuts)if(cut>=start&&cut<=end)cuts.push_back(cut+delta);
        std::fill(t.left.begin()+a,t.left.begin()+finish,0);std::fill(t.right.begin()+a,t.right.begin()+finish,0);t.left.resize(std::max(t.left.size(),at+n),0);t.right.resize(t.left.size(),0);std::copy(l.begin(),l.end(),t.left.begin()+at);std::copy(r.begin(),r.end(),t.right.begin()+at);
        t.cuts.erase(std::remove_if(t.cuts.begin(),t.cuts.end(),[&](double cut){return (cut>=start&&cut<=end)||(cut>=at/sr&&cut<=(at+n)/sr);}),t.cuts.end());t.cuts.insert(t.cuts.end(),cuts.begin(),cuts.end());t.cuts.push_back(at/sr);t.cuts.push_back((at+n)/sr);std::sort(t.cuts.begin(),t.cuts.end());}
}
void Engine::bounceTracks(unsigned mask,int destination,double start,double end,bool clearSources){
    mask=mask?mask&15u:1u<<selected;
    if(!(mask&15u)||destination<0||destination>3||!std::isfinite(start)||!std::isfinite(end)||start>=end)return;
    // Reuse the bounded control-thread clipboard renderer, preserving the user's clipboard/history.
    auto saved=std::move(clipboard);const auto savedMask=clipboardMask;const auto savedFrames=clipboardFrames;const double head=position;
    copyTracks(mask,start,end);if(clipboardMask){snapshot((clearSources?mask:0)|(1u<<destination));position=std::clamp(start,0.,360.)*sr;
        writeClipboardMix(destination);
        if(clearSources)for(int i=0;i<4;++i)if(i!=destination&&(mask&(1u<<i))){auto& t=tape[i];const size_t a=std::min(t.left.size(),static_cast<size_t>(std::clamp(start,0.,360.)*sr)),b=std::min(t.left.size(),static_cast<size_t>(std::clamp(end,0.,360.)*sr));std::fill(t.left.begin()+a,t.left.begin()+b,0);std::fill(t.right.begin()+a,t.right.begin()+b,0);t.cuts.erase(std::remove_if(t.cuts.begin(),t.cuts.end(),[&](double c){return c>=start&&c<=end;}),t.cuts.end());}
    }clipboard=std::move(saved);clipboardMask=savedMask;clipboardFrames=savedFrames;position=head;
}
void Engine::setTapeRoll(bool held,float beats){if(held&&!rollHeld){rollBegin=position;rollLength=std::max(1.,sr*60./params.tempo*clamp(beats,.03125f,8.f));}rollHeld=held;}
void Engine::split(){snapshot();auto& cuts=tape[selected].cuts;cuts.push_back(position/sr);std::sort(cuts.begin(),cuts.end());cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());}
void Engine::restore(Snapshot&& s){selected=s.track;for(int i=0;i<4;++i)if(s.mask&(1u<<i))tape[i]=std::move(s.audio[i]);position=s.head;isRecording=false;isPlaying=false;countInRemaining=0;reverseHeld=false;brakeHeld=false;rollHeld=false;}
void Engine::undo(){if(undos.empty())return;selected=undos.back().track;redos.push_back(capture(undos.back().mask));auto s=std::move(undos.back());undos.pop_back();restore(std::move(s));trimHistory();}
void Engine::redo(){if(redos.empty())return;selected=redos.back().track;undos.push_back(capture(redos.back().mask));auto s=std::move(redos.back());redos.pop_back();restore(std::move(s));trimHistory();}
std::vector<float> Engine::waveform(int track,int bins)const{
    std::vector<float> result(static_cast<size_t>(std::max(0,bins)),0);if(track<0||track>=4||bins<=0)return result;
    const auto&t=tape[track];if(t.left.empty())return result;
    // Display-only decimation: at most 32 samples per bin, independent of
    // tape duration. This is a waveform overview, not a true-peak analyser.
    for(int b=0;b<bins;++b){
        size_t a=static_cast<size_t>(static_cast<double>(b)/bins*t.left.size());
        size_t e=std::min(t.left.size(),std::max(a+1,static_cast<size_t>(static_cast<double>(b+1)/bins*t.left.size())));
        size_t count=std::min<size_t>(32,e-a);float v=0;
        for(size_t j=0;j<count;++j){size_t i=a+(e-a-1)*j/std::max<size_t>(1,count-1);v=std::max(v,std::max(std::abs(t.left[i]),std::abs(t.right[i])));}
        result[static_cast<size_t>(b)]=v;
    }
    return result;
}
std::pair<std::vector<float>,std::vector<float>> Engine::exportMix()const{
    size_t n=0;for(const auto&t:tape)n=std::max(n,t.left.size());std::pair<std::vector<float>,std::vector<float>> out{std::vector<float>(n,0),std::vector<float>(n,0)};
    bool solo=std::any_of(tape.begin(),tape.end(),[](const Track&t){return t.solo;});
    for(const auto&t:tape){if(t.mute||(solo&&!t.solo))continue;float l=t.level*(t.pan>0?1-t.pan:1),r=t.level*(t.pan<0?1+t.pan:1);for(size_t i=0;i<t.left.size();++i){out.first[i]+=t.left[i]*l;out.second[i]+=t.right[i]*r;}}
    for(size_t i=0;i<n;++i){
        auto colour=[&](float v){
            switch(params.tapeStyle){case 1:return std::tanh(v*1.25f)*.8f;case 2:return std::tanh(v*1.7f)*.65f;case 3:return std::round(v*8192)/8192;default:return v;}
        };
        out.first[i]=std::clamp(colour(out.first[i])*params.volume,-1.0f,1.0f);
        out.second[i]=std::clamp(colour(out.second[i])*params.volume,-1.0f,1.0f);
    }return out;
}
void Engine::restoredTape(){
    for(auto&t:tape){const size_t n=std::min(t.left.size(),maxFrames());t.left.resize(n);t.right.resize(n,0);for(auto&v:t.left)v=clamp(v,-4,4);for(auto&v:t.right)v=clamp(v,-4,4);t.level=clamp(t.level,0,2);t.pan=clamp(t.pan,-1,1);}
    undos.clear();redos.clear();isPlaying=false;isRecording=false;countInRemaining=0;countInElapsed=0;reverseHeld=false;brakeHeld=false;rollHeld=false;position=std::clamp(position,0.0,static_cast<double>(maxFrames()-1));
}
void Engine::setSequenceStep(int step,int note,float velocity){if(step>=0&&step<999){steps[step]=std::clamp(note,-1,127);stepVelocity[step]=clamp(velocity);}}
int Engine::sequenceStep(int step)const{return step>=0&&step<999?steps[step]:-1;}
float Engine::sequenceVelocity(int step)const{return step>=0&&step<999?stepVelocity[step]:0;}
void Engine::setSongPosition(int sixteenthNotes){
    const int length=params.sequencer==3?32:seqLength;
    seqStep=std::clamp(sixteenthNotes,0,16383)%length;
    seqPositionPending=seqStep;seqPhase=0;seqLastNote=-1;
    clearSequenceGates();
}
void Engine::setSequenceLength(int count){seqLength=std::clamp(count,1,999);if(seqStep>=seqLength){seqStep=-1;seqPositionPending=-1;seqPhase=0;}}
void Engine::clearHeldNotes(){allNotesOff();}
void Engine::setSketchPoint(int index,float x,float y){if(index>=0&&index<256)sketch[static_cast<size_t>(index)]={clamp(x),clamp(y)};}
std::pair<float,float> Engine::sketchPoint(int index)const{const auto& p=sketch[static_cast<size_t>(std::clamp(index,0,255))];return {p.x,p.y};}
void Engine::setSketchLength(int length){sketchCount=std::clamp(length,2,256);}
void Engine::setTombolaBall(int index,const BallState& source){
    if(index<0||index>=24)return;
    auto b=source;b.x=clamp(b.x,.018f,.982f);b.y=clamp(b.y,.018f,.982f);b.vx=clamp(b.vx,-8,8);b.vy=clamp(b.vy,-8,8);b.note=std::clamp(b.note,0,127);balls[static_cast<size_t>(index)]=b;ballVelocity[static_cast<size_t>(index)]=.8f;ballCooldown[static_cast<size_t>(index)]=0;
}
void Engine::setTombolaRotation(float radians){ballAngle=std::isfinite(radians)?std::remainder(radians,static_cast<float>(tau)):0;}
void Engine::setFingerPattern(int pattern){fingerSelected=std::clamp(pattern,0,15);}
void Engine::setFingerStep(int pattern,int step,int note,float velocity){if(pattern>=0&&pattern<16&&step>=0&&step<32){fingerBanks[static_cast<size_t>(pattern)].note[static_cast<size_t>(step)]=std::clamp(note,-1,127);fingerBanks[static_cast<size_t>(pattern)].velocity[static_cast<size_t>(step)]=clamp(velocity);}}
int Engine::fingerStep(int pattern,int step)const{return pattern>=0&&pattern<16&&step>=0&&step<32?fingerBanks[static_cast<size_t>(pattern)].note[static_cast<size_t>(step)]:-1;}
float Engine::fingerVelocity(int pattern,int step)const{return pattern>=0&&pattern<16&&step>=0&&step<32?fingerBanks[static_cast<size_t>(pattern)].velocity[static_cast<size_t>(step)]:0;}
void Engine::setFingerLength(int pattern,int length){if(pattern>=0&&pattern<16)fingerBanks[static_cast<size_t>(pattern)].length=std::clamp(length,1,32);}
int Engine::fingerLength(int pattern)const{return fingerBanks[static_cast<size_t>(std::clamp(pattern,0,15))].length;}
void Engine::setPitchBend(float value){pitchBend=clamp(std::isfinite(value)?value:0,-48,48);}
float Engine::voiceRoute(int target,float velocity) const {
    if(params.lfo==5)return params.lfoTarget==target?(velocity*2-1)*params.lfoDepth:0;
    return modulationRoutes[static_cast<size_t>(target)];
}
void Engine::setMidiCC(int cc,float value){if(cc>=1&&cc<=4){midiControllers[static_cast<size_t>(cc-1)]=clamp(value);if(cc==1)modWheel=clamp(value);}}
void Engine::setModWheel(float value){setMidiCC(1,value);}
void Engine::setOutputGainImmediately(float value){smoothedVolume=clamp(value,0,2.f);}
}
