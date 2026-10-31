#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PresetFormat.h"
#include "PresetMapping.h"
#include "PresetParameters.h"
#include <cmath>
#include <cstring>
#include <limits>

const juce::StringArray FieldProcessor::engineNames{"Voltage","FM","Cluster","Digital","Dr Wave","Dimension","String","Pulse","Phase","DNA","DSynth","Sampler","Vocoder","Amp","DBox"};
const juce::StringArray FieldProcessor::effectNames{"Off","Delay","Spring","Grid","Nitro","Phone","Punch","CWO","Terminal","Mother","Fazer"};
const juce::StringArray FieldProcessor::sequencerNames{"Off","Arpeggio","Endless","Finger","Hold","Pattern","Sketch","Tombola"};
const juce::StringArray FieldProcessor::lfoTargetNames{"Pitch","Blue macro","Ochre macro","Grey macro","Orange macro","Source volume","Source pan","Source filter","Attack","Decay","Sustain","Release","FX blue","FX ochre","FX grey","FX orange","FX mix","Patch gain","Tape speed","Main volume"};
const juce::StringArray FieldProcessor::lfoNames{"Random","Element","MIDI","Tremolo","Value","Velocity"};

juce::AudioProcessorValueTreeState::ParameterLayout FieldProcessor::makeLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto f=[&](const char* id,const char* name,float min,float max,float def,float skew=1.f){
        l.add(std::make_unique<AudioParameterFloat>(ParameterID{id,1},name,NormalisableRange<float>(min,max,0.f,skew),def));
    };
    auto c=[&](const char* id,const char* name,const StringArray& names,int def=0){l.add(std::make_unique<AudioParameterChoice>(ParameterID{id,1},name,names,def));};
    c("engine","Engine",engineNames);c("voiceMode","Voice mode",{"Poly","Mono","Legato","Unison"});f("glideTime","Portamento",0,2,0,.4f);f("bendRange","Pitch bend range",1,12,2);f("patchGain","Patch volume",0,1,1);
    f("macro1","Blue",0,1,.35f); f("macro2","Ochre",0,1,.5f); f("macro3","Grey",0,1,.45f); f("macro4","Orange",0,1,.3f);
    f("attack","Attack",.001f,4,.01f,.3f);f("decay","Decay",.005f,4,.2f,.3f);f("sustain","Sustain",0,1,.7f);f("release","Release",.005f,8,.25f,.3f);
    f("sampleRootNote","Sample root MIDI note",0,127,60);
    f("sampleStart","Sample start",0,1,0);f("sampleLoopIn","Sample loop in",0,1,0);f("sampleLoopOut","Sample loop out",0,1,1);f("sampleEnd","Sample end",0,1,1);
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"sampleLoop",1},"Sample loop",false));l.add(std::make_unique<AudioParameterBool>(ParameterID{"sampleReverse",1},"Sample reverse",false));l.add(std::make_unique<AudioParameterBool>(ParameterID{"sampleOneShot",1},"Sample one shot",false));
    for(int i=1;i<=4;++i){juce::String id="fx"+juce::String(i);l.add(std::make_unique<AudioParameterFloat>(ParameterID{id,1},"Effect macro "+juce::String(i),0.f,1.f,.3f));juce::String sid="seq"+juce::String(i);l.add(std::make_unique<AudioParameterFloat>(ParameterID{sid,1},"Sequencer macro "+juce::String(i),0.f,1.f,.3f));}
    c("lfoTarget","LFO target",lfoTargetNames);
    c("lfoWaveform","LFO waveform",{"Sine","Triangle","Saw","Square","Exponential","Blip"});
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"lfoRetrigger",1},"LFO key retrigger",false));l.add(std::make_unique<AudioParameterBool>(ParameterID{"lfoSync",1},"LFO tempo sync",false));
    c("lfoDivision","LFO cycle length",{"16 beats","8 beats","4 beats","2 beats","1 beat","3/4 beat","1/2 beat","3/8 beat","1/4 beat","3/16 beat","1/8 beat","1/16 beat"},4);
    c("elementSource","Element source",{"Audio input","Voice envelope","Main output","Manual control"});f("elementManual","Element manual value",0,1,.5f);
    c("envelopeCurve","Envelope curve",{"Linear","Exponential"});
    for(int i=1;i<=4;++i){c(("midiTarget"+juce::String(i)).toRawUTF8(),("CC "+juce::String(i)+" destination").toRawUTF8(),lfoTargetNames,i-1);f(("midiDepth"+juce::String(i)).toRawUTF8(),("CC "+juce::String(i)+" amount").toRawUTF8(),-1,1,0);}

    c("effect","Effect",effectNames);f("effectMix","Effect mix",0,1,.2f);
    f("masterLow","Master low",-24,12,0);f("masterMid","Master mid",-24,12,0);f("masterHigh","Master high",-24,12,0);f("masterDrive","Master drive",0,1,0);f("compressor","Master compressor",0,1,0);
    for(int i=1;i<=4;++i)l.add(std::make_unique<AudioParameterFloat>(ParameterID{"masterFx"+juce::String(i),1},"Master effect macro "+juce::String(i),0.f,1.f,.3f));
    c("masterEffect","Master effect",effectNames);f("masterMix","Master effect mix",0,1,.2f);
    f("volume","Volume",0,2,.65f);f("tempo","Tempo",30,240,110);
    c("lfo","LFO",lfoNames);f("lfoRate","LFO rate",.02f,20,1,.4f);f("lfoDepth","LFO amount",0,1,0);
    f("seqSwing","Sequence swing",0,.5f,0);f("seqGate","Sequence gate",.05f,1,.65f);l.add(std::make_unique<AudioParameterBool>(ParameterID{"seqHold",1},"Sequence hold",false));
    c("sequencer","Sequencer",sequencerNames);l.add(std::make_unique<AudioParameterBool>(ParameterID{"sequenceOn",1},"Sequencer on",false));
    juce::StringArray channels{"Omni"};for(int i=1;i<=16;++i)channels.add("Channel "+juce::String(i));c("midiInputChannel","MIDI input channel",channels);
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"midiOutputChannel",1},"MIDI output channel",1,16,1));
    c("midiClockMode","MIDI clock",{"Internal","Receive MIDI clock","Send MIDI clock"});
    for(auto option:{std::pair<const char*,bool>{"midiThru",false},{"midiSendControls",false},{"midiRelativeControls",false},{"midiNotesIn",true},{"midiOtherIn",true},{"midiTransportIn",true},{"midiNotesOut",true},{"midiOtherOut",true}})l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{option.first,1},option.first,option.second));
    f("midiSppOrigin","MIDI SPP origin seconds",0,360,0);f("metronomeLevel","Metronome level",0,1,1);
    f("masterLeft","Master left",0,1,1);f("masterRight","Master right",0,1,1);f("compressorRelease","Master drive release",.005f,2,.12f,.35f);
    c("tapeGrid","Tape edit grid",{"Bar (4 beats)","Half bar (2 beats)","Beat","Eighth note"},2);
    c("tapeStyle","Tape style",{"Studio","Vintage","Porta","Disc Mini"});
    f("tapeSpeed","Tape speed",-2,2,1);f("tapeRecordLevel","Tape record level",0,2,1);f("tapeRecordPan","Tape record pan",-1,1,0);
    l.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"countInBeats",1},"Record count-in beats",0,8,0));
    l.add(std::make_unique<AudioParameterBool>(ParameterID{"loop",1},"Loop",false));
    return l;
}
namespace {
struct MirrorNotification {const FieldProcessor* owner=nullptr;int key=-1;float value=0;};
thread_local MirrorNotification activeMirror;
}
FieldProcessor::FieldProcessor() : AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true).withOutput("Tape 1",juce::AudioChannelSet::stereo(),false).withOutput("Tape 2",juce::AudioChannelSet::stereo(),false).withOutput("Tape 3",juce::AudioChannelSet::stereo(),false).withOutput("Tape 4",juce::AudioChannelSet::stereo(),false)), parameters(*this,nullptr,"PARAMETERS",makeLayout()) { formats.registerBasicFormats();const char* ids[]{"engine","macro1","macro2","macro3","macro4","attack","decay","sustain","release","sampleStart","sampleLoopIn","sampleLoopOut","sampleEnd","sampleLoop","sampleReverse","sampleOneShot","fx1","fx2","fx3","fx4","seq1","seq2","seq3","seq4","lfoTarget","effect","effectMix","masterLow","masterMid","masterHigh","masterDrive","compressor","masterEffect","masterMix","volume","tempo","lfo","lfoRate","lfoDepth","sequencer","sequenceOn","tapeStyle","tapeSpeed","loop","masterFx1","masterFx2","masterFx3","masterFx4","seqSwing","seqGate","seqHold","sampleRootNote","voiceMode","glideTime","bendRange","patchGain","lfoWaveform","lfoRetrigger","lfoSync","lfoDivision","elementSource","elementManual","envelopeCurve","midiTarget1","midiTarget2","midiTarget3","midiTarget4","midiDepth1","midiDepth2","midiDepth3","midiDepth4","countInBeats","tapeRecordLevel","tapeRecordPan","metronomeLevel","midiInputChannel","midiOutputChannel","midiClockMode","midiThru","midiSendControls","midiRelativeControls","midiNotesIn","midiOtherIn","midiTransportIn","midiNotesOut","midiOtherOut","midiSppOrigin","masterLeft","masterRight","compressorRelease"};for(int i=0;i<P_count;++i){rawValues[(size_t)i]=parameters.getRawParameterValue(ids[i]);rawTargets[(size_t)i]=parameters.getParameter(ids[i]);jassert(rawValues[(size_t)i]!=nullptr);parameterIds[(size_t)i]=ids[i];realtimeValues.initialize((size_t)i,rawValues[(size_t)i]->load());rawTargets[(size_t)i]->addListener(this);}startTimerHz(60); }
FieldProcessor::~FieldProcessor(){stopTimer();for(auto* parameter:rawTargets)parameter->removeListener(this);}
void FieldProcessor::parameterValueChanged(int index,float normalized){for(int i=0;i<P_count;++i)if(rawTargets[(size_t)i]->getParameterIndex()==index){const auto version=realtimeValues.version((size_t)i);const float actual=cached(i),received=rawTargets[(size_t)i]->convertFrom0to1(normalized);const float tolerance=8*std::numeric_limits<float>::epsilon()*std::max(1.f,std::abs(actual));if(std::abs(actual-received)>tolerance){realtimeValues.fromAudio((size_t)i,actual);return;}if(activeMirror.owner==this&&activeMirror.key==i&&std::abs(activeMirror.value-received)<=tolerance)return;realtimeValues.fromControlIfUnchanged((size_t)i,version,actual);return;}}
juce::ValueTree FieldProcessor::copyParameterState(){auto copy=parameters.copyState();for(auto item:copy)if(item.hasProperty("id")){const auto id=item["id"].toString();if(parameters.getParameter(id))item.setProperty("value",value(id),nullptr);}return copy;}
void FieldProcessor::flushParameterMirrors(){for(int i=0;i<P_count;++i){field::RealtimeValues<P_count>::Snapshot snapshot;if(realtimeValues.pending((size_t)i,snapshot)){struct RestoreMirror {MirrorNotification previous;~RestoreMirror(){activeMirror=previous;}} restore{activeMirror};const float current=cached(i);activeMirror={this,i,current};auto* p=rawTargets[(size_t)i];p->sendValueChangedMessageToListeners(p->convertTo0to1(current));realtimeValues.acknowledge((size_t)i,snapshot);}}}
float FieldProcessor::value(const juce::String& id) const {for(int i=0;i<P_count;++i)if(parameterIds[(size_t)i]==id)return cached(i);auto* p=parameters.getRawParameterValue(id);return p?p->load():0;}
void FieldProcessor::setValue(const juce::String& id,float v) {if(auto* p=parameters.getParameter(id)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();}}
field::Parameters FieldProcessor::currentParameters() const {
    field::Parameters p;
    p.voiceMode=(int)cached(P_voiceMode);p.glideTime=cached(P_glideTime);p.patchGain=cached(P_patchGain);p.engine=(int)cached(P_engine); for(int i=0;i<4;++i)p.macro[i]=cached(P_macro1+i);
    p.attack=cached(P_attack);p.decay=cached(P_decay);p.sustain=cached(P_sustain);p.release=cached(P_release);
    for(int i=0;i<4;++i){p.effectMacro[i]=cached(P_fx1+i);p.sequencerMacro[i]=cached(P_seq1+i);}p.lfoTarget=(int)cached(P_lfoTarget);
    p.effect=(int)cached(P_effect);p.effectMix=cached(P_effectMix);p.volume=cached(P_volume);p.tapeRecordLevel=cached(P_tapeRecordLevel);p.tapeRecordPan=cached(P_tapeRecordPan);p.tempo=cached(P_tempo);
    p.lfoWaveform=(int)cached(P_lfoWaveform);p.lfoRetrigger=cached(P_lfoRetrigger)>.5f;p.lfoSync=cached(P_lfoSync)>.5f;p.lfoDivision=(int)cached(P_lfoDivision);p.elementSource=(int)cached(P_elementSource);p.elementManual=cached(P_elementManual);p.envelopeCurve=(int)cached(P_envelopeCurve);
    for(int i=0;i<4;++i){p.midiLfoTargets[(size_t)i]=(int)cached(P_midiTarget1+i);p.midiLfoDepth[(size_t)i]=cached(P_midiDepth1+i);}
    p.lfo=(int)cached(P_lfo);p.lfoRate=cached(P_lfoRate);p.lfoDepth=cached(P_lfoDepth);p.sequencer=(int)cached(P_sequencer);p.sequenceOn=cached(P_sequenceOn)>.5f;p.seqSwing=cached(P_seqSwing);p.seqGate=cached(P_seqGate);p.seqHold=cached(P_seqHold)>.5f;
    p.tapeStyle=(int)cached(P_tapeStyle);p.tapeSpeed=cached(P_tapeSpeed);p.loop=cached(P_loop)>.5f;
    p.drumMode=drumMode;p.monitorInput=inputMonitor;p.metronome=metronome;p.metronomeLevel=cached(P_metronomeLevel);
    if(hostSync)p.tempo=effectiveTempo.load();
    if(!hostSync&&(int)cached(P_midiClockMode)==1&&midiClockFollower.ready())p.tempo=(float)midiClockFollower.bpm();
    p.sampleRootNote=cached(P_sampleRootNote);p.sampleStart=cached(P_sampleStart);p.sampleLoopIn=cached(P_sampleLoopIn);p.sampleLoopOut=cached(P_sampleLoopOut);p.sampleEnd=cached(P_sampleEnd);p.sampleLoop=cached(P_sampleLoop)>.5f;p.sampleReverse=cached(P_sampleReverse)>.5f;p.sampleOneShot=cached(P_sampleOneShot)>.5f;
    return p;
}
void FieldProcessor::updateEngine(){if((int)cached(P_engine)==14)drumMode=true;engine.setParameters(currentParameters());engine.setCountInBeats((int)cached(P_countInBeats));}
void FieldProcessor::prepareToPlay(double sr,int block){const juce::ScopedLock lock(engineLock);
    double old=engine.sampleRate();albumRecording=-1;albumPlaying=-1;engine.prepare(sr,block);masterFx.prepare(sr,block);masterFx.setOutputGainImmediately(1);masterState={};sampling=false;captureAudio=field::TapeAudio{};captureExhausted=false;
    if(old!=sr)for(auto& bank:tapes)for(auto& t:bank.tracks)t.audio.resample(sr/old,(size_t)(sr*360));
    midiClockFollower.prepare(sr);midiClockGenerator.prepare(sr);missedReleases.discardPending();missedReleaseOverflow=false;midiNotes.resetAll();guiMidiNotes.resetAll();midiSampleClock=0;midiPositionValid=false;midiTransportPpq=0;lastSppOrigin=cached(P_midiSppOrigin);midiOutput.ensureSize(65536);midiInput.ensureSize(65536);keyboard.clearIncoming();lastClockMode=(int)cached(P_midiClockMode);lastNotesIn=cached(P_midiNotesIn)>.5f;lastNotesOut=cached(P_midiNotesOut)>.5f;lastInputChannel=(int)cached(P_midiInputChannel);lastOutputChannel=(int)cached(P_midiOutputChannel);controllerValues.fill(-1);
    inputCopy.setSize(2,juce::jmax(block,8192));updateEngine();
}
bool FieldProcessor::isBusesLayoutSupported(const BusesLayout& l) const {if(l.getMainOutputChannelSet()!=juce::AudioChannelSet::stereo())return false;if(!l.getMainInputChannelSet().isDisabled()&&l.getMainInputChannelSet()!=juce::AudioChannelSet::mono()&&l.getMainInputChannelSet()!=juce::AudioChannelSet::stereo())return false;for(int i=1;i<l.outputBuses.size();++i)if(!l.outputBuses[i].isDisabled()&&l.outputBuses[i]!=juce::AudioChannelSet::stereo())return false;return true;}
void FieldProcessor::retainSkippedReleases(const juce::MidiBuffer& midi)noexcept{int inspected=0;for(const auto event:midi){if(++inspected>2048){missedReleaseOverflow=true;break;}if(event.numBytes!=3)continue;const auto* data=event.data;const auto kind=data[0]&0xf0;const bool release=kind==0x80||(kind==0x90&&data[2]==0)||(kind==0xb0&&((data[1]==64&&data[2]<64)||data[1]==120||data[1]==123));if(release&&!missedReleases.push({data[0],data[1],data[2]}))missedReleaseOverflow=true;}}
void FieldProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    if(buffer.getNumSamples()<=0){retainSkippedReleases(midi);midi.clear();return;}
    const juce::ScopedTryLock lock(engineLock);
    if(!lock.isLocked()){retainSkippedReleases(midi);buffer.clear();midi.clear();midiSampleClock+=buffer.getNumSamples();++skippedBlocks;return;}
    midiOutput.clear();engine.beginMidiBlock();midiClockFollower.advance(midiSampleClock);updateEngine();
    const int inputChannel=(int)cached(P_midiInputChannel),outputChannel=(int)cached(P_midiOutputChannel);
    if((int)cached(P_midiClockMode)!=lastClockMode){if(lastClockMode==2)midiOutput.addEvent(juce::MidiMessage::midiStop(),0);lastClockMode=(int)cached(P_midiClockMode);midiClockFollower.reset();midiClockGenerator.reset();updateEngine();}
    if(inputChannel!=lastInputChannel||(cached(P_midiNotesIn)>.5f)!=lastNotesIn){engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();lastInputChannel=inputChannel;lastNotesIn=cached(P_midiNotesIn)>.5f;keyboard.clearIncoming();midiOutput.addEvent(juce::MidiMessage::allNotesOff(outputChannel),0);}
    if((cached(P_midiNotesOut)>.5f)!=lastNotesOut){midiOutput.addEvent(juce::MidiMessage::allNotesOff(outputChannel),0);lastNotesOut=cached(P_midiNotesOut)>.5f;}
    if(outputChannel!=lastOutputChannel){midiOutput.addEvent(juce::MidiMessage::allNotesOff(lastOutputChannel),0);lastOutputChannel=outputChannel;controllerValues.fill(-1);}
    if(hostSync) if(auto* ph=getPlayHead())if(auto pos=ph->getPosition()) {
        if(auto bpm=pos->getBpm()){auto p=currentParameters();p.tempo=(float)*bpm;engine.setParameters(p);}
        bool running=pos->getIsPlaying();
        engine.setPlaying(running);
        if(auto seconds=pos->getTimeInSeconds()){double expected=previousHostSeconds+(previousHostPlaying?previousHostFrames/engine.sampleRate():0);if(!hostPositionValid||std::abs(expected-*seconds)>2/engine.sampleRate())engine.seek(*seconds);previousHostSeconds=*seconds;hostPositionValid=true;previousHostFrames=buffer.getNumSamples();}previousHostPlaying=running;
    }
    effectiveTempo.store(engine.parameters().tempo);if(!hostSync)hostPositionValid=false;
    // Copy only the input bus; surplus output channels are never read as input.
    const int n=buffer.getNumSamples();
    if(n>inputCopy.getNumSamples()){retainSkippedReleases(midi);buffer.clear();midi.clear();midiSampleClock+=n;++skippedBlocks;return;}
    const int inputs=getTotalNumInputChannels();
    for(int c=0;c<2;++c){if(inputs>0)inputCopy.copyFrom(c,0,buffer,juce::jmin(c,inputs-1),0,n);else inputCopy.clear(c,0,n);}
    if(sampling&&!captureFromOutput){const size_t at=captureAudio.size(),count=std::min((size_t)n,captureLimit>at?captureLimit-at:0);for(size_t f=0;f<count;++f)if(!captureAudio.tryWrite(at+f,inputCopy.getSample(0,(int)f),inputCopy.getSample(1,(int)f))){captureExhausted=true;break;}}
    // One bounded GUI producer feeds the audio consumer. Never invoke JUCE's
    // MidiKeyboardState lock/listeners from the callback.
    midiInput.clear();int inspected=0;bool inputOverflow=false;int forwarded=0;
    if(missedReleaseOverflow){missedReleaseOverflow=false;missedReleases.discardPending();engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();keyboard.clearIncoming();midiOutput.addEvent(juce::MidiMessage::allNotesOff(outputChannel),0);}
    // Recovered releases precede newer GUI/host notes, including on MIDI out.
    field::KeyboardEvent savedRelease;for(int i=0;i<512&&missedReleases.pop(savedRelease);++i){const int channel=(savedRelease.status&15)+1,kind=savedRelease.status&0xf0;if(cached(P_midiNotesIn)<.5f||(inputChannel!=0&&channel!=inputChannel))continue;
        keyboard.observe(savedRelease.status,savedRelease.data1,savedRelease.data2);++midiReceived;midiLastStatus=savedRelease.status;midiLastData1=savedRelease.data1;midiLastData2=savedRelease.data2;
        if(kind==0x80||(kind==0x90&&savedRelease.data2==0)){if(midiNotes.noteOff(channel,savedRelease.data1)&&!guiMidiNotes.heldAnywhere(savedRelease.data1))engine.noteOff(savedRelease.data1);}
        else if(kind==0xb0&&savedRelease.data1==64)midiController(channel,64,savedRelease.data2);
        else if(kind==0xb0)clearMidiChannel(channel);
        if((cached(P_midiThru)>.5f||cached(P_midiSendControls)>.5f)&&cached(P_midiNotesOut)>.5f){const uint8_t data[]{(uint8_t)(kind|(outputChannel-1)),savedRelease.data1,savedRelease.data2};midiOutput.addEvent(data,3,0);++forwarded;}
    }
    for(const auto event:midi){if(++inspected>2048){inputOverflow=true;++rejectedMidiEvents;break;}if(event.numBytes<=0||event.numBytes>3){++rejectedMidiEvents;continue;}midiInput.addEvent(event.data,event.numBytes,juce::jlimit(0,n,event.samplePosition));}
    if(inputOverflow){engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();keyboard.clearIncoming();midiInput.clear();midiOutput.addEvent(juce::MidiMessage::allNotesOff(outputChannel),0);}
    unsigned inputEncoderMask=0;
    auto handleHostEvent=[&](const juce::MidiMessage& msg,int sample){
        ++midiReceived;const auto* raw=msg.getRawData();midiLastStatus=raw[0];midiLastData1=msg.getRawDataSize()>1?raw[1]:0;midiLastData2=msg.getRawDataSize()>2?raw[2]:0;
        const bool transportMessage=msg.isMidiClock()||msg.isMidiStart()||msg.isMidiContinue()||msg.isMidiStop()||msg.isSongPositionPointer();
        const bool noteMessage=msg.isNoteOnOrOff()||msg.isAllNotesOff()||msg.isAllSoundOff()||(msg.isController()&&msg.getControllerNumber()==64);
        if(msg.getChannel()>0&&inputChannel!=0&&msg.getChannel()!=inputChannel)return;
        if((transportMessage&&cached(P_midiTransportIn)<.5f)||(noteMessage&&cached(P_midiNotesIn)<.5f)||(!transportMessage&&!noteMessage&&cached(P_midiOtherIn)<.5f))return;
        keyboard.observe(raw[0],msg.getRawDataSize()>1?raw[1]:0,msg.getRawDataSize()>2?raw[2]:0);
        if((cached(P_midiThru)>.5f||cached(P_midiSendControls)>.5f)&&!msg.isMidiClock()&&!(transportMessage&&(int)cached(P_midiClockMode)==2)&&!msg.isSysEx()&&forwarded<2048&&sample<n&&(noteMessage?cached(P_midiNotesOut)>.5f:cached(P_midiOtherOut)>.5f)){auto routed=msg;if(routed.getChannel()>0)routed.setChannel(outputChannel);midiOutput.addEvent(routed,sample);++forwarded;}
        if(msg.isNoteOn()){if(midiNotes.noteOn(msg.getChannel(),msg.getNoteNumber()))engine.noteOn(msg.getNoteNumber(),msg.getFloatVelocity());}
        else if(msg.isNoteOff()){if(midiNotes.noteOff(msg.getChannel(),msg.getNoteNumber())&&!guiMidiNotes.heldAnywhere(msg.getNoteNumber()))engine.noteOff(msg.getNoteNumber());}
        else if(msg.isAllNotesOff()||msg.isAllSoundOff())clearMidiChannel(msg.getChannel());
        else if(msg.isPitchWheel())engine.setPitchBend((msg.getPitchWheelValue()-8192)/8192.f*cached(P_bendRange));
        else if(msg.isController()){const int cc=msg.getControllerNumber();if(cc>=46&&cc<=49)inputEncoderMask|=1u<<(cc-46);midiController(msg.getChannel(),cc,msg.getControllerValue());}
        else if(msg.isProgramChange()&&msg.getProgramChangeNumber()<16)pendingProgram.store(msg.getProgramChangeNumber());
        else if(msg.isMidiClock()){if(!hostSync&&(int)cached(P_midiClockMode)==1){midiClockFollower.observe(msg,midiSampleClock+sample);updateEngine();}}
        else if(msg.isMidiStart()){midiClockFollower.observe(msg,midiSampleClock+sample);engine.rewind();engine.setSongPosition(0);engine.setPlaying(true);}
        else if(msg.isMidiContinue()){midiClockFollower.observe(msg,midiSampleClock+sample);engine.setPlaying(true);}
        else if(msg.isMidiStop()){midiClockFollower.observe(msg,midiSampleClock+sample);engine.setPlaying(false);}
        else if(msg.isSongPositionPointer()){midiClockFollower.observe(msg,midiSampleClock+sample);engine.setSongPosition(msg.getSongPositionPointerMidiBeat());midiTransportPpq=msg.getSongPositionPointerMidiBeat()/4.;midiExpectedTapeSeconds=engine.positionSeconds();midiPositionValid=true;}
    };
    for(const auto item:midiInput){if(item.samplePosition>0)break;handleHostEvent(item.getMessage(),0);}
    field::KeyboardEvent guiEvent;for(int i=0;i<512&&keyboard.pop(guiEvent);++i){
        if(guiEvent.status==0){engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();keyboard.clearIncoming();midiOutput.addEvent(juce::MidiMessage::allNotesOff(outputChannel),0);continue;}
        const int channel=(guiEvent.status&15)+1,kind=guiEvent.status&0xf0;
        if(cached(P_midiNotesIn)<.5f||(inputChannel!=0&&channel!=inputChannel))continue;
        ++midiReceived;midiLastStatus=guiEvent.status;midiLastData1=guiEvent.data1;midiLastData2=guiEvent.data2;
        if(kind==0x90&&guiEvent.data2>0){if(guiMidiNotes.noteOn(channel,guiEvent.data1))engine.noteOn(guiEvent.data1,guiEvent.data2/127.f);}
        else if(kind==0x80||(kind==0x90&&guiEvent.data2==0)){if(guiMidiNotes.noteOff(channel,guiEvent.data1)&&!midiNotes.heldAnywhere(guiEvent.data1))engine.noteOff(guiEvent.data1);}
        else if(kind==0xb0&&guiEvent.data1==123)clearMidiChannel(channel);
        if((cached(P_midiThru)>.5f||cached(P_midiSendControls)>.5f)&&cached(P_midiNotesOut)>.5f&&forwarded<2048){const uint8_t data[]{(uint8_t)(kind|(outputChannel-1)),guiEvent.data1,guiEvent.data2};midiOutput.addEvent(data,3,0);++forwarded;}
    }
    buffer.clear();
    int offset=0;
    auto render=[&](int end){if(end>offset){const double head=engine.positionSeconds(),bpm=engine.parameters().tempo,origin=cached(P_midiSppOrigin);if(!midiPositionValid||std::abs(head-midiExpectedTapeSeconds)>2/engine.sampleRate()||origin!=lastSppOrigin){midiTransportPpq=std::max(0.,head-origin)*bpm/60.;lastSppOrigin=origin;midiPositionValid=true;}midiClockGenerator.process(midiOutput,end-offset,bpm,(int)cached(P_midiClockMode)==2,engine.playing(),midiTransportPpq,(int)cached(P_midiClockMode)==1,offset);if(engine.playing())midiTransportPpq+=(end-offset)*bpm/(60.*engine.sampleRate());float* stems[8]{};for(int i=0;i<4;++i){auto stem=getBusBuffer(buffer,false,i+1);if(stem.getNumChannels()==2){stems[i*2]=stem.getWritePointer(0)+offset;stems[i*2+1]=stem.getWritePointer(1)+offset;}}engine.process(buffer.getWritePointer(0)+offset,buffer.getWritePointer(1)+offset,inputCopy.getReadPointer(0)+offset,inputCopy.getReadPointer(1)+offset,end-offset,stems);midiExpectedTapeSeconds=engine.positionSeconds();offset=end;}};
    for(const auto item:midiInput){int sample=juce::jlimit(0,n,item.samplePosition);if(sample==0)continue;render(sample);handleHostEvent(item.getMessage(),sample);}
    render(n);
    if(cached(P_midiNotesOut)>.5f)for(size_t i=0;i<engine.midiEventCount();++i){const auto& event=engine.midiEvents()[i];auto msg=event.note<0?juce::MidiMessage::allNotesOff(outputChannel):event.on?juce::MidiMessage::noteOn(outputChannel,event.note,event.velocity):juce::MidiMessage::noteOff(outputChannel,event.note);midiOutput.addEvent(msg,juce::jlimit(0,n-1,event.frame));}
    if(forwarded>=2048&&cached(P_midiNotesOut)>.5f)midiOutput.addEvent(juce::MidiMessage::allNotesOff(outputChannel),n-1);
    if(cached(P_midiSendControls)>.5f&&cached(P_midiOtherOut)>.5f){for(int i=0;i<4;++i){const int next=juce::jlimit(0,127,(int)std::lround(cached(P_macro1+i)*127));int& previous=controllerValues[(size_t)i];if(inputEncoderMask&(1u<<i)){previous=next;continue;}if(next!=previous){if(cached(P_midiRelativeControls)>.5f){if(previous>=0){int delta=next-previous;while(delta){int step=juce::jlimit(-63,63,delta);midiOutput.addEvent(juce::MidiMessage::controllerEvent(outputChannel,46+i,step<0?128+step:step),0);delta-=step;}}}else midiOutput.addEvent(juce::MidiMessage::controllerEvent(outputChannel,46+i,next),0);previous=next;}}}else controllerValues.fill(-1);
    midiSampleClock+=n;effectiveTempo=engine.parameters().tempo;
    processMaster(buffer.getWritePointer(0),buffer.getWritePointer(1),n,engine.sampleRate(),masterState,masterFx);
    int listening=albumPlaying.load();if(listening>=0){auto& side=albums[(size_t)listening];for(int i=0;i<n;++i){if(albumHead>=side.audio.size()){albumPlaying=-1;buffer.clear(0,i,n-i);buffer.clear(1,i,n-i);break;}buffer.setSample(0,i,side.audio.interpolate(0,albumHead));buffer.setSample(1,i,side.audio.interpolate(1,albumHead));albumHead+=side.rate/engine.sampleRate();}}
    int album=albumRecording.load();if(album>=0){auto& side=albums[(size_t)album];const size_t at=side.audio.size(),count=std::min((size_t)n,albumLimit>at?albumLimit-at:0);for(size_t f=0;f<count;++f)if(!side.audio.tryWrite(at+f,buffer.getSample(0,(int)f),buffer.getSample(1,(int)f))){albumRecording=-1;captureExhausted=true;break;}if(side.audio.size()>=albumLimit)albumRecording=-1;}
    if(sampling&&captureFromOutput){const size_t at=captureAudio.size(),count=std::min((size_t)n,captureLimit>at?captureLimit-at:0);for(size_t f=0;f<count;++f)if(!captureAudio.tryWrite(at+f,buffer.getSample(0,(int)f),buffer.getSample(1,(int)f))){captureExhausted=true;break;}}
    midi.clear();midi.addEvents(midiOutput,0,n,0);outputPeak.store(std::max(buffer.getMagnitude(0,0,n),buffer.getMagnitude(1,0,n)));playhead.store(engine.positionSeconds());
}
void FieldProcessor::timerCallback(){flushParameterMirrors();{const juce::ScopedTryLock lock(engineLock);if(lock.isLocked()){engine.collectTapeStorage(64);for(auto& bank:tapes)for(auto& track:bank.tracks)track.audio.collect(64);for(auto& side:albums)side.audio.collect(64);captureAudio.collect(64);}}{const juce::ScopedTryLock lock(sampleWorkerLock);if(lock.isLocked())engine.collectRetiredSamples();}int program=pendingProgram.exchange(-1);if(program>=0)setCurrentProgram(program);int action=pendingPatchAction.exchange(0);if(action)applyPatchAction(action);if(sampling){bool full=false;{const juce::ScopedTryLock lock(engineLock);if(lock.isLocked())full=captureAudio.size()>=captureLimit||captureExhausted.load();}if(full)stopSampleCapture();}}
void FieldProcessor::clearMidiChannel(int channel){if(channel<1||channel>16)return;const auto hostReleased=midiNotes.resetChannel(channel),guiReleased=guiMidiNotes.resetChannel(channel);std::array<bool,128> released{};for(int i=0;i<hostReleased.count;++i)released[(size_t)hostReleased.notes[(size_t)i]]=true;for(int i=0;i<guiReleased.count;++i)released[(size_t)guiReleased.notes[(size_t)i]]=true;for(int note=0;note<128;++note)if(released[(size_t)note]&&!midiNotes.heldAnywhere(note)&&!guiMidiNotes.heldAnywhere(note))engine.noteOff(note);keyboard.observe((uint8_t)(0xb0+channel-1),123,0);}
void FieldProcessor::midiController(int channel,int cc,int amount){
    if(midiLearn.process(channel,cc,amount,[this](size_t target){return mappedMidiValue(target);},[this](size_t target,float value){applyMappedMidi(target,value);})){updateEngine();return;}
    const float v=amount/127.f;const int track=juce::jlimit(0,3,channel-1);
    if(cc==64){const bool down=amount>=64;const auto hostReleased=midiNotes.controllerSustain(channel,down),guiReleased=guiMidiNotes.controllerSustain(channel,down);std::array<bool,128> released{};for(int i=0;i<hostReleased.count;++i)released[(size_t)hostReleased.notes[(size_t)i]]=true;for(int i=0;i<guiReleased.count;++i)released[(size_t)guiReleased.notes[(size_t)i]]=true;for(int note=0;note<128;++note)if(released[(size_t)note]&&!midiNotes.heldAnywhere(note)&&!guiMidiNotes.heldAnywhere(note))engine.noteOff(note);}
    else if(cc>=1&&cc<=4)engine.setMidiCC(cc,v);
    else if(cc==7&&channel<=4)engine.setTrackLevel(track,v);
    else if(cc==9&&channel<=4)engine.setTrackMute(track,amount>=64);
    else if(cc==10&&channel<=4)engine.setTrackPan(track,v*2-1);
    else if(cc>=46&&cc<=49){if(drumMode&&(int)cached(P_engine)==14){int i=engine.selectedPad();auto pad=engine.dboxPad(i);if(cc==46)pad.mix=v;else if(cc==47)pad.wave1=v;else if(cc==48)pad.envelope1=.005f*std::pow(400.f,v);else pad.crossMod=v;engine.setDboxPad(i,pad);}else if(drumMode&&(int)cached(P_engine)==11){int i=engine.selectedPad();auto pad=engine.drumPad(i);if(cc==46)pad.tune=(v-.5f)*48;else if(cc==47)pad.start=v;else if(cc==48)pad.end=v;else pad.mode=std::min(2,(int)(v*3));engine.setDrumPad(i,pad);}else setNormalized(P_macro1+cc-46,v);}
    else if(cc>=50&&cc<=53){if(drumMode&&(int)cached(P_engine)==14){int i=engine.selectedPad();auto pad=engine.dboxPad(i);if(cc==50)pad.attack=.001f*std::pow(2000.f,v);else if(cc==51)pad.gain=v*2;else if(cc==52)pad.release=.005f*std::pow(400.f,v);else pad.smooth=v;engine.setDboxPad(i,pad);}else setNormalized(P_attack+cc-50,v);}
    else if(cc>=54&&cc<=57)setNormalized(P_fx1+cc-54,v);
    else if(cc>=58&&cc<=61){const int type=(int)cached(P_lfo);if(type==2)setPhysical(P_midiDepth1+cc-58,v*2-1);else if(cc==58)setNormalized(P_lfoRate,v);else if(cc==59)setNormalized(P_lfoDepth,v);else if(cc==60)setNormalized(P_lfoTarget,v);else if(type==1)setNormalized(P_elementManual,v);else setNormalized(P_lfoWaveform,v);}
    else if(cc==62&&amount>=64)pendingPatchAction=1;else if(cc==63&&amount>=64)pendingPatchAction=2;
    else if(cc>=70&&cc<=73)setNormalized(P_masterFx1+cc-70,v);
    else if(cc==74)setNormalized(P_masterLeft,v);else if(cc==75)setNormalized(P_masterRight,v);else if(cc==76)setNormalized(P_compressor,v);else if(cc==77)setNormalized(P_compressorRelease,v);
    else if(cc==78)setNormalized(P_tapeRecordLevel,v);else if(cc==79)octave=juce::jlimit(-3,3,octave.load()+(amount<64?-1:1));else if(cc==81)setNormalized(P_metronomeLevel,v);else if(cc==102&&channel<=8&&amount>=64)pendingProgram=channel-1+(drumMode?8:0);
    else if(cc>=90&&cc<=92){setNormalized(P_masterLow+cc-90,v);}
    else if(cc==93)drumMode=amount>=64;
    else if(cc==80){float bpm=amount<=5?40.f+2*amount:amount<=120?52.f+(amount-6):168.f+2*(amount-121);setPhysical(P_tempo,bpm);}
    else if(amount>=64){
        if(cc==82)engine.seek(std::max(0.,engine.positionSeconds()-240./engine.parameters().tempo));
        else if(cc==83)engine.seek(engine.positionSeconds()+240./engine.parameters().tempo);
        else if(cc==84)engine.rewind();else if(cc==85)engine.seek(engine.recordedSeconds());
        else if(cc==86){selectionStart=engine.positionSeconds();selectionEnd=std::max(selectionEnd,selectionStart+.1);engine.setLoop(selectionStart,selectionEnd);}
        else if(cc==87){selectionEnd=std::max(selectionStart+.1,engine.positionSeconds());engine.setLoop(selectionStart,selectionEnd);}
        else if(cc==88)setPhysical(P_loop,cached(P_loop)>.5f?0.f:1.f);
        else if(cc==104)engine.setPlaying(false);else if(cc==105)engine.setPlaying(true);
    }
    updateEngine();
}
juce::AudioProcessorEditor* FieldProcessor::createEditor(){return new FieldEditor(*this);}
void FieldProcessor::setCurrentProgram(int i){const juce::ScopedLock lock(engineLock);i=juce::jlimit(0,15,i);drumMode=i>=8;int slot=i;i%=8;if(patchSlots[(size_t)slot].isValid()){recallPatchSlot(i);return;}currentProgram=i;engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();if(drumMode){setValue("engine",14);engine.resetDboxPads();for(int pad=0;pad<24;++pad){auto sound=engine.dboxPad(pad);sound.mix=.15f+((i*13+pad*7)%70)*.01f;sound.crossMod=i*.07f;sound.wave2=(i%5)*.2f;sound.cutoff=.35f+i*.08f;engine.setDboxPad(pad,sound);}}else setValue("engine",(float)std::array<int,8>{0,1,5,6,9,4,7,10}[(size_t)i]);for(int k=0;k<4;++k)setValue("macro"+juce::String(k+1),.15f+float((i*17+k*23)%70)*.01f);setValue("effect",(float)(i%3));updateEngine();}
const juce::String FieldProcessor::getProgramName(int i){i=juce::jlimit(0,15,i);if(i>=8)return "Drum kit "+juce::String(i-7);return juce::StringArray{"First light","Glass FM","Wide horizon","Plucked wire","Genetic drift","Wave study","Pulse train","Digital dusk"}[i];}
void FieldProcessor::applyPatchAction(int action){const juce::ScopedLock lock(engineLock);engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();juce::Random random;for(auto* base:getParameters())if(auto* p=dynamic_cast<juce::RangedAudioParameter*>(base)){const auto id=p->paramID;const bool sound=id.startsWith("macro")||id=="attack"||id=="decay"||id=="sustain"||id=="release"||id.startsWith("fx")||id=="effectMix"||id.startsWith("lfo")||id.startsWith("midiDepth")||id.startsWith("midiTarget")||id.startsWith("element")||id=="voiceMode"||id=="glideTime"||id=="patchGain";if(sound)p->setValueNotifyingHost(action==1?random.nextFloat():p->getDefaultValue());}updateEngine();}
void FieldProcessor::resetSampleParameters(){for(auto* base:getParameters())if(auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(base)){auto id=parameter->paramID;if(id.startsWith("lfo")||id.startsWith("midiTarget")||id.startsWith("midiDepth")||id.startsWith("element")||id=="envelopeCurve")parameter->setValueNotifyingHost(parameter->getDefaultValue());}

    setValue("voiceMode",0);setValue("glideTime",0);setValue("bendRange",2);setValue("patchGain",1);setValue("sampleStart",0);setValue("sampleLoopIn",0);setValue("sampleLoopOut",1);setValue("sampleEnd",1);setValue("sampleRootNote",60);setValue("sampleLoop",0);setValue("sampleReverse",0);setValue("sampleOneShot",0);setValue("effect",0);setValue("effectMix",.2f);for(int i=1;i<=4;++i)setValue("fx"+juce::String(i),.3f);setValue("lfoDepth",0);setValue("attack",.001f);setValue("decay",.01f);setValue("sustain",1);setValue("release",.03f);
}
void FieldProcessor::loadLegacySample(const std::vector<float>& left,const std::vector<float>& right,double rate){const juce::ScopedLock worker(sampleWorkerLock);engine.loadSample(left,right,rate);}
bool FieldProcessor::loadSampleFile(const juce::File& file,juce::String& error){
    juce::String metadataError;auto metadata=field::preset::readMetadata(file,metadataError);
    if(metadataError.isNotEmpty()){error=metadataError;return false;}
    const bool preset=field::preset::isPreset(metadata);auto type=metadata.getProperty("type",{}).toString().trim().toLowerCase();
    const bool drum=type=="drum",sampled=drum||type=="sampler";
    const int importedEngine=field::preset::engineIndex(metadata);
    if(preset&&!sampled&&importedEngine<0){error="Unsupported OP-1 preset engine: "+type+". The current sound was kept.";return false;}
    std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(file));
    if(!r){error="Unsupported audio file. Use WAV, AIFF, FLAC or Ogg.";return false;}
    if(!std::isfinite(r->sampleRate)||r->sampleRate<8000||r->sampleRate>192000){error="Choose audio sampled between 8 kHz and 192 kHz.";return false;}
    if(r->lengthInSamples<=0||r->lengthInSamples>r->sampleRate*120){error="Choose audio between 0 and 120 seconds.";return false;}
    field::preset::ParameterTranslation translation;if(preset&&!field::preset::translateParameters(metadata,translation,error))return false;
    field::preset::SampleMapping mapping;if(!field::preset::sampleMapping(metadata,r->lengthInSamples/r->sampleRate,mapping,error))return false;
    juce::AudioBuffer<float> audio(2,(int)r->lengthInSamples);if(!r->read(&audio,0,audio.getNumSamples(),0,true,true)){error="Could not read audio.";return false;}
    std::vector<float> l(audio.getReadPointer(0),audio.getReadPointer(0)+audio.getNumSamples()),rr(audio.getReadPointer(r->numChannels>1?1:0),audio.getReadPointer(r->numChannels>1?1:0)+audio.getNumSamples());
    auto prepared=field::Engine::prepareSample(l,rr,r->sampleRate);std::vector<float> retiredLeft,retiredRight;
    const juce::ScopedLock lock(engineLock);
    sampling=false;captureAudio=field::TapeAudio{};captureLimit=0;{const juce::ScopedLock worker(sampleWorkerLock);auto* handle=engine.registerPreparedSample(std::move(prepared));engine.installPreparedSample(handle);}retiredLeft.swap(sampleLeft);retiredRight.swap(sampleRight);sampleLeft=std::move(l);sampleRight=std::move(rr);sampleRateFile=r->sampleRate;sampleName=file.getFileName();importedPresetMetadata=metadata;
    resetSampleParameters();engine.resetDrumPads();
    if(mapping.sampled){
        drumMode=drum;setValue("engine",11);
        if(mapping.hasMarkers){const char* ids[]{"sampleStart","sampleLoopIn","sampleLoopOut","sampleEnd"};for(int i=0;i<4;++i)setValue(ids[i],mapping.markers[(size_t)i]);}
        if(mapping.hasRoot)setValue("sampleRootNote",mapping.rootNote);
        if(drum)for(int i=0;i<24;++i){auto p=mapping.pads[(size_t)i];field::DrumPad pad;pad.start=p.start;pad.end=p.end;pad.tune=p.tune;pad.level=p.level;pad.pan=p.pan;pad.reverse=p.reverse;pad.mode=p.mode;engine.setDrumPad(i,pad);}
        lastImportSummary=drum?"OP-1 drum kit imported with 24 pad regions":"OP-1 sample patch imported";
        lastImportDetails=mapping.profile+"\nSample audio retained at its original sample rate.\n"+mapping.warnings.joinIntoString("\n");
        if(!mapping.warnings.isEmpty())lastImportSummary+=" (see COM > Import details)";
    }else if(preset){
        setValue("engine",(float)importedEngine);drumMode=type=="dbox";if(drumMode)engine.resetDboxPads();setValue("attack",.01f);setValue("decay",.2f);setValue("sustain",.7f);setValue("release",.25f);
        for(int i=1;i<=4;++i){auto id="macro"+juce::String(i);auto* p=parameters.getParameter(id);setValue(id,p->convertFrom0to1(p->getDefaultValue()));}
        lastImportSummary="OP-1 "+type+" controls translated; exact sound is not supported";
        lastImportDetails="Known control positions and selectors are transferred into the software engine. Original raw values remain attached. Timing, gain and DSP-response equivalence are unverified; see the detailed mapping report. Embedded preview audio is not treated as a substitute for the modeled synth sound.";
    }else {setValue("engine",11);lastImportSummary="Audio imported: "+file.getFileName();lastImportDetails="Ordinary audio loaded with neutral sampler settings. The current synth/drum mode was retained.";}
    if(preset){for(const auto& parameter:translation.parameterValues)setValue(parameter.id,parameter.value);lastImportDetails+="\n\nCONTROL TRANSLATION\n"+translation.sourceProfile+"\n"+translation.mapped.joinIntoString("\n")+"\n\nUNVERIFIED / RETAINED\n"+translation.unsupported.joinIntoString("\n");}
    updateEngine();return true;
}
bool FieldProcessor::exportAudio(const juce::File& file,juce::String& error){
    std::array<field::Track,4> tracks;field::Parameters mixParameters;double rate;
    {const juce::ScopedLock lock(engineLock);tracks=engine.tracks();mixParameters=engine.parameters();rate=engine.sampleRate();}
    auto mix=field::Engine::exportMix(tracks,mixParameters);
    if(mix.first.empty()){error="Record to tape before exporting.";return false;}
    MasterState master;auto offlineFx=std::make_unique<field::Engine>();offlineFx->prepare(rate,512);offlineFx->setOutputGainImmediately(1);processMaster(mix.first.data(),mix.second.data(),(int)mix.first.size(),rate,master,*offlineFx);
    juce::TemporaryFile tmp(file);auto stream=tmp.getFile().createOutputStream();if(!stream){error="Cannot write destination.";return false;}
    juce::WavAudioFormat wav;
    auto* raw=stream.release();
    std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(raw,rate,2,24,{},0));
    if(!writer){delete raw;error="Cannot create WAV.";return false;}
    const float* channels[]{mix.first.data(),mix.second.data()};bool ok=writer->writeFromFloatArrays(channels,2,(int)mix.first.size());writer.reset();
    if(!ok||!tmp.overwriteTargetFileWithTemporary()){error="Could not finish writing WAV.";return false;}return true;
}
void FieldProcessor::startSampleCapture(bool fromOutput){const juce::ScopedLock lock(engineLock);engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();sampleLeft.clear();sampleRight.clear();sampleRateFile=engine.sampleRate();size_t count=(size_t)(sampleRateFile*(drumMode?20:6));captureLimit=count;captureAudio=field::TapeAudio{};captureAudio.prepareWrite(count);captureExhausted=false;sampling=true;captureFromOutput=fromOutput;sampleName=fromOutput?"Output resample":"Input recording";}
void FieldProcessor::stopSampleCapture(){const juce::ScopedLock lock(engineLock);if(!sampling)return;sampling=false;sampleLeft=captureAudio.exportChannel(0);sampleRight=captureAudio.exportChannel(1);captureAudio=field::TapeAudio{};loadLegacySample(sampleLeft,sampleRight,sampleRateFile);resetSampleParameters();importedPresetMetadata=juce::var{};lastImportSummary="Sample captured";lastImportDetails="Captured audio uses neutral sampler settings.";setValue("engine",11);}
void FieldProcessor::processMaster(float* l,float* r,int n,double sr,MasterState& state,field::Engine& fx){
    const int effect=(int)cached(P_masterEffect);
    if(effect>0){field::Parameters p;p.monitorInput=true;p.volume=1;p.effect=effect;p.effectMix=cached(P_masterMix);p.tempo=effectiveTempo.load();for(int i=0;i<4;++i)p.effectMacro[i]=cached(P_masterFx1+i);fx.setParameters(p);fx.process(l,r,l,r,n);}
    const float lowGain=std::pow(10.f,cached(P_masterLow)/20),midGain=std::pow(10.f,cached(P_masterMid)/20),highGain=std::pow(10.f,cached(P_masterHigh)/20);
    const float lowCoeff=1-std::exp(-2*3.14159265358979323846f*200.f/(float)sr),upperCoeff=1-std::exp(-2*3.14159265358979323846f*3500.f/(float)sr);
    const float leftGain=cached(P_masterLeft),rightGain=cached(P_masterRight);
    const float drive=cached(P_masterDrive),comp=cached(P_compressor),release=std::exp(-1.f/((float)sr*cached(P_compressorRelease)));
    for(int i=0;i<n;++i){float samples[2]{l[i],r[i]};for(int c=0;c<2;++c){state.low[c]+=lowCoeff*(samples[c]-state.low[c]);state.upper[c]+=upperCoeff*(samples[c]-state.upper[c]);samples[c]=state.low[c]*lowGain+(state.upper[c]-state.low[c])*midGain+(samples[c]-state.upper[c])*highGain;if(drive>0)samples[c]=std::tanh(samples[c]*(1+drive*8))/std::tanh(1+drive*8);}
        float level=std::max(std::abs(samples[0]),std::abs(samples[1]));state.envelope=std::max(level,release*state.envelope);
        float threshold=.8f-comp*.65f;float gain=comp>0&&state.envelope>threshold?std::pow(threshold/state.envelope,comp*.75f):1;
        l[i]=juce::jlimit(-1.f,1.f,samples[0]*gain*leftGain);r[i]=juce::jlimit(-1.f,1.f,samples[1]*gain*rightGain);
    }
}
void FieldProcessor::startAlbumRecording(int side){const juce::ScopedLock lock(engineLock);side=juce::jlimit(0,1,side);albumPlaying=-1;albumHead=0;auto& a=albums[(size_t)side];a.audio=field::TapeAudio{};a.rate=engine.sampleRate();albumLimit=(size_t)(a.rate*360);a.audio.prepareWrite(albumLimit);captureExhausted=false;albumRecording=side;}
bool FieldProcessor::playAlbum(int side){const juce::ScopedLock lock(engineLock);side=juce::jlimit(0,1,side);if(albums[(size_t)side].audio.empty())return false;albumRecording=-1;engine.setPlaying(false);engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();albumHead=0;albumPlaying=side;return true;}
void FieldProcessor::stopAlbum(){const juce::ScopedLock lock(engineLock);albumRecording=-1;albumPlaying=-1;}
void FieldProcessor::clearAlbum(int side){const juce::ScopedLock lock(engineLock);side=juce::jlimit(0,1,side);if(albumPlaying==side)albumPlaying=-1;if(albumRecording==side)albumRecording=-1;albums[(size_t)side]=AlbumSide{};}
double FieldProcessor::albumDuration(int side) const {const juce::ScopedLock lock(engineLock);const auto& a=albums[(size_t)juce::jlimit(0,1,side)];return a.audio.size()/a.rate;}
bool FieldProcessor::exportAlbum(int side,const juce::File& file,juce::String& error){AlbumSide audio;{const juce::ScopedLock lock(engineLock);audio=albums[(size_t)juce::jlimit(0,1,side)];}if(audio.audio.empty()){error="Record this album side before exporting.";return false;}juce::TemporaryFile temp(file);auto stream=temp.getFile().createOutputStream();if(!stream){error="Cannot write destination.";return false;}auto* raw=stream.release();juce::WavAudioFormat wav;std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(raw,audio.rate,2,24,{},0));if(!writer){delete raw;error="Cannot create album WAV.";return false;}auto left=audio.audio.exportChannel(0),right=audio.audio.exportChannel(1);const float* channels[]{left.data(),right.data()};bool ok=writer->writeFromFloatArrays(channels,2,(int)left.size());writer.reset();if(!ok||!temp.overwriteTargetFileWithTemporary()){error="Could not finish writing album WAV.";return false;}return true;}
void FieldProcessor::transport(const juce::String& action){const juce::ScopedLock lock(engineLock);
    if(action=="play")engine.setPlaying(!engine.playing());
    else if(action=="stop"){albumRecording=-1;albumPlaying=-1;engine.setRecording(false);engine.setPlaying(false);engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();engine.setTapeReverse(false);engine.setTapeBrake(false);engine.setTapeRoll(false);}
    else if(action=="record"){updateEngine();engine.setRecording(!engine.recording());}
    else if(action=="rewind")engine.rewind();
    else if(action=="undo")engine.undo();else if(action=="redo")engine.redo();
    else if(action=="copy")engine.copyTracks(tapeEditMask.load(),selectionStart,selectionEnd);
    else if(action=="lift")engine.liftTracks(tapeEditMask.load(),selectionStart,selectionEnd);
    else if(action=="drop")engine.dropTracks();else if(action=="split")engine.split();
    else if(action=="merge")engine.mergeClipboard();
    else if(action=="join")engine.joinTracks(tapeEditMask.load(),selectionStart,selectionEnd);
    else if(action=="slideBack"||action=="slideForward"){double delta=60./std::max(30.f,(hostSync||value("midiClockMode")==1)?effectiveTempo.load():value("tempo"))*(action=="slideBack"?-1:1);delta=juce::jlimit(-selectionStart,360.-selectionEnd,delta);engine.slideTracks(tapeEditMask.load(),selectionStart,selectionEnd,delta);selectionStart+=delta;selectionEnd+=delta;engine.setLoop(selectionStart,selectionEnd);}
    else if(action=="bounce")engine.bounceTracks(tapeEditMask.load(),engine.selectedTrack(),selectionStart,selectionEnd,false);
    else if(action=="clear")engine.clearTrack();else if(action=="back")engine.seek(std::max(0.,engine.positionSeconds()-1));
    else if(action=="forward")engine.seek(engine.positionSeconds()+1);
    else if(action=="in"){selectionStart=engine.positionSeconds();selectionEnd=std::max(selectionEnd,selectionStart+.1);engine.setLoop(selectionStart,selectionEnd);}
    else if(action=="out"){selectionEnd=std::max(selectionStart+.1,engine.positionSeconds());engine.setLoop(selectionStart,selectionEnd);}
    else if(action=="panic"){keyboard.allNotesOff(0);engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();}
}
double FieldProcessor::snapTapeTime(double seconds)const {seconds=juce::jlimit(0.,360.,seconds);if(!tapeSnap)return seconds;const double divisions[]{4,2,1,.5};const double beat=60./std::max(30.f,(hostSync||value("midiClockMode")==1)?effectiveTempo.load():value("tempo"))*divisions[juce::jlimit(0,3,(int)value("tapeGrid"))];return juce::jlimit(0.,360.,std::round(seconds/beat)*beat);}
void FieldProcessor::setTapePerformance(int control,bool down){const juce::ScopedLock lock(engineLock);if(control==0)engine.setTapeBrake(down);else if(control==1)engine.setTapeReverse(down);else if(control==2)engine.setTapeRoll(down,.25f);}
void FieldProcessor::releaseTapePerformance(){const juce::ScopedLock lock(engineLock);engine.setTapeBrake(false);engine.setTapeReverse(false);engine.setTapeRoll(false);}
void FieldProcessor::setTrack(int i){const juce::ScopedLock lock(engineLock);engine.setRecording(false);engine.selectTrack(i);}
void FieldProcessor::selectTape(int i){const juce::ScopedLock lock(engineLock);i=juce::jlimit(0,7,i);if(i==currentTape)return;engine.setRecording(false);engine.setPlaying(false);tapes[(size_t)currentTape].position=engine.positionSeconds();tapes[(size_t)currentTape].tracks=std::move(engine.tracks());currentTape=i;engine.tracks()=std::move(tapes[(size_t)i].tracks);engine.restoredTape();engine.seek(tapes[(size_t)i].position);}
void FieldProcessor::newProject(){const juce::ScopedLock lock(engineLock);midiLearn.clearAll();const juce::ScopedLock worker(sampleWorkerLock);midiClockFollower.reset();midiClockGenerator.reset();midiPositionValid=false;controllerValues.fill(-1);engine.setRecording(false);engine.setPlaying(false);engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();for(auto& t:tapes)t=TapeState{};for(auto& a:albums)a=AlbumSide{};albumRecording=-1;albumPlaying=-1;albumHead=0;for(auto& slot:patchSlots)slot={};for(auto& memo:memos)memo={};engine.tracks()={};engine.restoredTape();engine.rewind();currentTape=0;currentProgram=0;drumMode=false;double currentRate=getSampleRate()>0?getSampleRate():engine.sampleRate();auto fresh=std::make_unique<field::Engine>();engine=std::move(*fresh);engine.prepare(currentRate,juce::jmax(512,getBlockSize()));engine.resetDrumPads();engine.resetDboxPads();engine.setSequenceLength(16);for(int i=0;i<999;++i)engine.setSequenceStep(i,-1);const int initial[]{60,-1,67,-1,63,-1,70,67,60,-1,75,-1,70,67,63,-1};for(int i=0;i<16;++i)engine.setSequenceStep(i,initial[i]);masterFx.prepare(engine.sampleRate(),512);masterFx.setOutputGainImmediately(1);masterState={};selectionStart=0;selectionEnd=4;tapeEditMask=0;tapeSnap=false;engine.setLoop(0,4);projectName="Untitled";sampleLeft.clear();sampleRight.clear();loadLegacySample(sampleLeft,sampleRight,44100);sampleName="No sample loaded";sampling=false;captureAudio=field::TapeAudio{};captureExhausted=false;importedPresetMetadata=juce::var{};lastImportSummary.clear();lastImportDetails.clear();for(auto* parameter:getParameters())if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(parameter))ranged->setValueNotifyingHost(ranged->getDefaultValue());}

namespace {
juce::MemoryBlock packFloats(const std::vector<float>& f){return juce::MemoryBlock(f.data(),f.size()*sizeof(float));}
void restoreDbox(field::Engine& engine,const juce::var& value){if(auto* rows=value.getArray())for(int i=0;i<std::min(24,rows->size());++i)if(auto* row=(*rows)[i].getArray())if(row->size()==12){field::DBoxPad p;p.mix=(float)(*row)[0];p.wave1=(float)(*row)[1];p.envelope1=(float)(*row)[2];p.crossMod=(float)(*row)[3];p.frequency=(float)(*row)[4];p.wave2=(float)(*row)[5];p.envelope2=(float)(*row)[6];p.cutoff=(float)(*row)[7];p.attack=(float)(*row)[8];p.gain=(float)(*row)[9];p.release=(float)(*row)[10];p.smooth=(float)(*row)[11];engine.setDboxPad(i,p);}}
juce::ValueTree readCompressedState(juce::InputStream& input){juce::GZIPDecompressorInputStream zipped(input);juce::MemoryBlock raw;std::array<char,65536> chunk{};constexpr size_t max=512u*1024u*1024u;for(;;){int count=zipped.read(chunk.data(),(int)chunk.size());if(count<=0)break;if(raw.getSize()+(size_t)count>max)throw std::runtime_error("Project decompressed size exceeds 512 MB");raw.append(chunk.data(),(size_t)count);}return juce::ValueTree::readFromData(raw.getData(),raw.getSize());}
std::vector<float> unpackFloats(const juce::var& v,size_t limit){auto* b=v.getBinaryData();if(!b||b->getSize()%4!=0||b->getSize()/4>limit)throw std::runtime_error("Invalid audio payload");std::vector<float> f(b->getSize()/4);std::memcpy(f.data(),b->getData(),b->getSize());for(auto& s:f)if(!std::isfinite(s))s=0;return f;}
}
void FieldProcessor::storeMemo(int i){const juce::ScopedLock lock(engineLock);i=juce::jlimit(0,1,i);juce::ValueTree memo("MEMO");memo.addChild(copyParameterState(),-1,nullptr);for(int j=0;j<4;++j){juce::ValueTree track("TRACK");track.setProperty("level",engine.trackLevel(j),nullptr);track.setProperty("pan",engine.trackPan(j),nullptr);track.setProperty("mute",engine.trackMute(j),nullptr);track.setProperty("solo",engine.trackSolo(j),nullptr);memo.addChild(track,-1,nullptr);}memos[(size_t)i]=memo;}
void FieldProcessor::recallMemo(int i){const juce::ScopedLock lock(engineLock);auto memo=memos[(size_t)juce::jlimit(0,1,i)];if(!memo.isValid())return;for(auto param:memo.getChildWithName("PARAMETERS")){auto id=param["id"].toString();if(id.startsWith("master")||id.startsWith("compressor")||id=="volume"||id=="tapeSpeed")if(auto* target=parameters.getParameter(id))target->setValueNotifyingHost(target->convertTo0to1((float)param["value"]));}int j=0;for(auto track:memo)if(track.hasType("TRACK")&&j<4){engine.setTrackLevel(j,(float)track["level"]);engine.setTrackPan(j,(float)track["pan"]);engine.setTrackMute(j,(bool)track["mute"]);engine.setTrackSolo(j,(bool)track["solo"]);++j;}updateEngine();}
void FieldProcessor::savePatchSlot(int slot){
    const juce::ScopedLock lock(engineLock);slot=juce::jlimit(0,7,slot)+(drumMode?8:0);juce::ValueTree patch("PATCH");patch.addChild(copyParameterState(),-1,nullptr);patch.setProperty("left",packFloats(sampleLeft),nullptr);patch.setProperty("right",packFloats(sampleRight),nullptr);patch.setProperty("rate",sampleRateFile,nullptr);patch.setProperty("name",sampleName,nullptr);patch.setProperty("metadata",juce::JSON::toString(importedPresetMetadata),nullptr);patch.setProperty("importSummary",lastImportSummary,nullptr);patch.setProperty("importDetails",lastImportDetails,nullptr);juce::Array<juce::var> pads;for(int i=0;i<24;++i){auto p=engine.drumPad(i);juce::Array<juce::var> row{p.start,p.end,p.tune,p.level,p.pan,p.reverse,p.mode,p.chokeGroup};pads.add(row);}patch.setProperty("pads",pads,nullptr);juce::Array<juce::var> dbox;for(int i=0;i<24;++i){auto p=engine.dboxPad(i);juce::Array<juce::var> row{p.mix,p.wave1,p.envelope1,p.crossMod,p.frequency,p.wave2,p.envelope2,p.cutoff,p.attack,p.gain,p.release,p.smooth};dbox.add(row);}patch.setProperty("dbox",dbox,nullptr);patchSlots[(size_t)slot]=patch;currentProgram=slot%8;
}
void FieldProcessor::recallPatchSlot(int slot){
    const juce::ScopedLock lock(engineLock);slot=juce::jlimit(0,7,slot)+(drumMode?8:0);auto patch=patchSlots[(size_t)slot];if(!patch.isValid())return;engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();
    for(auto param:patch.getChildWithName("PARAMETERS")){auto id=param["id"].toString();bool allowed=id=="engine"||id=="voiceMode"||id=="glideTime"||id=="bendRange"||id=="patchGain"||id.startsWith("macro")||id=="attack"||id=="decay"||id=="sustain"||id=="release"||id=="effect"||id=="effectMix"||id.startsWith("fx")||id.startsWith("lfo")||id.startsWith("midiTarget")||id.startsWith("midiDepth")||id.startsWith("element")||id=="envelopeCurve"||id.startsWith("sample");if(allowed)if(auto* target=parameters.getParameter(id))target->setValueNotifyingHost(target->convertTo0to1((float)param["value"]));}
    double rate=(double)patch["rate"];try{if(!std::isfinite(rate)||rate<8000||rate>192000)throw std::runtime_error("Invalid patch sample rate");auto l=unpackFloats(patch["left"],(size_t)(rate*120)),r=unpackFloats(patch["right"],(size_t)(rate*120));sampleLeft=std::move(l);sampleRight=std::move(r);sampleRateFile=rate;sampleName=patch["name"].toString();importedPresetMetadata=juce::JSON::parse(patch["metadata"].toString());lastImportSummary=patch["importSummary"].toString();lastImportDetails=patch["importDetails"].toString();loadLegacySample(sampleLeft,sampleRight,rate);auto pads=patch["pads"];if(auto* all=pads.getArray())for(int i=0;i<std::min(24,all->size());++i){if(auto* row=(*all)[i].getArray())if(row->size()==8){field::DrumPad pad;pad.start=(float)(*row)[0];pad.end=(float)(*row)[1];pad.tune=(float)(*row)[2];pad.level=(float)(*row)[3];pad.pan=(float)(*row)[4];pad.reverse=(bool)(*row)[5];pad.mode=(int)(*row)[6];pad.chokeGroup=(int)(*row)[7];engine.setDrumPad(i,pad);}}restoreDbox(engine,patch["dbox"]);}catch(const std::exception&){}currentProgram=slot%8;updateEngine();
}
juce::ValueTree FieldProcessor::makeState(){
    std::array<TapeState,8> tapeSnapshot;std::array<AlbumSide,2> albumSnapshot;field::TapeAudio captureSnapshot;bool savingCapture=false;juce::ValueTree root("FIELD_STUDIO");
    {const juce::ScopedLock lock(engineLock);
    albumSnapshot=albums;savingCapture=sampling;captureSnapshot=captureAudio;
    tapeSnapshot=tapes;tapeSnapshot[(size_t)currentTape].tracks=engine.tracks();tapeSnapshot[(size_t)currentTape].position=engine.positionSeconds();
    root.setProperty("version",1,nullptr);root.setProperty("name",projectName,nullptr);root.setProperty("sampleRate",engine.sampleRate(),nullptr);root.setProperty("tape",currentTape,nullptr);root.setProperty("program",currentProgram,nullptr);root.setProperty("track",engine.selectedTrack(),nullptr);root.setProperty("position",engine.positionSeconds(),nullptr);root.setProperty("in",selectionStart,nullptr);root.setProperty("out",selectionEnd,nullptr);root.setProperty("drum",drumMode.load(),nullptr);root.setProperty("hostSync",hostSync.load(),nullptr);root.setProperty("metronome",metronome.load(),nullptr);root.setProperty("monitorInput",inputMonitor.load(),nullptr);root.setProperty("tapeEditMask",tapeEditMask.load(),nullptr);root.setProperty("tapeSnap",tapeSnap.load(),nullptr);
    root.addChild(copyParameterState(),-1,nullptr);root.addChild(midiMappingState(),-1,nullptr);root.setProperty("presetMetadata",juce::JSON::toString(importedPresetMetadata),nullptr);root.setProperty("importSummary",lastImportSummary,nullptr);root.setProperty("importDetails",lastImportDetails,nullptr);
    juce::ValueTree sample("SAMPLE");sample.setProperty("name",sampleName,nullptr);sample.setProperty("rate",sampleRateFile,nullptr);if(!savingCapture){sample.setProperty("left",packFloats(sampleLeft),nullptr);sample.setProperty("right",packFloats(sampleRight),nullptr);}root.addChild(sample,-1,nullptr);

    juce::Array<juce::var> steps;for(int i=0;i<999;++i)steps.add(engine.sequenceStep(i));root.setProperty("steps",steps,nullptr);root.setProperty("sequenceLength",engine.sequenceLength(),nullptr);

    juce::ValueTree sequencers("SEQUENCERS");sequencers.setProperty("sketchLength",engine.sketchLength(),nullptr);sequencers.setProperty("fingerPattern",engine.fingerPattern(),nullptr);sequencers.setProperty("fingerLength",engine.fingerLength(engine.fingerPattern()),nullptr);juce::Array<juce::var> sketch;for(int i=0;i<256;++i){auto p=engine.sketchPoint(i);juce::Array<juce::var> point{p.first,p.second};sketch.add(point);}sequencers.setProperty("sketch",sketch,nullptr);juce::Array<juce::var> finger,fingerLengths;for(int bank=0;bank<16;++bank){fingerLengths.add(engine.fingerLength(bank));juce::Array<juce::var> pattern;for(int step=0;step<32;++step){juce::Array<juce::var> data{engine.fingerStep(bank,step),engine.fingerVelocity(bank,step)};pattern.add(data);}finger.add(pattern);}sequencers.setProperty("finger",finger,nullptr);sequencers.setProperty("fingerLengths",fingerLengths,nullptr);root.addChild(sequencers,-1,nullptr);
    juce::ValueTree memories("MEMORIES");for(auto& memory:memos){juce::ValueTree m("ITEM");if(memory.isValid())m.addChild(memory.createCopy(),-1,nullptr);memories.addChild(m,-1,nullptr);}root.addChild(memories,-1,nullptr);
    juce::ValueTree slots("SLOTS");for(int i=0;i<16;++i){juce::ValueTree slot("SLOT");slot.setProperty("number",i,nullptr);if(patchSlots[(size_t)i].isValid())slot.addChild(patchSlots[(size_t)i].createCopy(),-1,nullptr);slots.addChild(slot,-1,nullptr);}root.addChild(slots,-1,nullptr);
    juce::ValueTree pads("PADS");pads.setProperty("selected",engine.selectedPad(),nullptr);for(int i=0;i<24;++i){auto p=engine.drumPad(i);juce::ValueTree pad("PAD");pad.setProperty("start",p.start,nullptr);pad.setProperty("end",p.end,nullptr);pad.setProperty("tune",p.tune,nullptr);pad.setProperty("level",p.level,nullptr);pad.setProperty("pan",p.pan,nullptr);pad.setProperty("reverse",p.reverse,nullptr);pad.setProperty("mode",p.mode,nullptr);pad.setProperty("choke",p.chokeGroup,nullptr);pads.addChild(pad,-1,nullptr);}root.addChild(pads,-1,nullptr);juce::Array<juce::var> dbox;for(int i=0;i<24;++i){auto p=engine.dboxPad(i);juce::Array<juce::var> row{p.mix,p.wave1,p.envelope1,p.crossMod,p.frequency,p.wave2,p.envelope2,p.cutoff,p.attack,p.gain,p.release,p.smooth};dbox.add(row);}root.setProperty("dbox",dbox,nullptr);}
    for(int i=0;i<8;++i){juce::ValueTree tape("TAPE");tape.setProperty("number",i,nullptr);tape.setProperty("position",tapeSnapshot[(size_t)i].position,nullptr);auto& tracks=tapeSnapshot[(size_t)i].tracks;
        for(auto& t:tracks){juce::ValueTree track("TRACK");track.setProperty("left",packFloats(t.audio.exportChannel(0)),nullptr);track.setProperty("right",packFloats(t.audio.exportChannel(1)),nullptr);track.setProperty("level",t.level,nullptr);track.setProperty("pan",t.pan,nullptr);track.setProperty("mute",t.mute,nullptr);track.setProperty("solo",t.solo,nullptr);juce::Array<juce::var> cuts;for(double c:t.cuts)cuts.add(c);track.setProperty("cuts",cuts,nullptr);tape.addChild(track,-1,nullptr);}root.addChild(tape,-1,nullptr);
    }
    if(savingCapture){auto sample=root.getChildWithName("SAMPLE");sample.setProperty("left",packFloats(captureSnapshot.exportChannel(0)),nullptr);sample.setProperty("right",packFloats(captureSnapshot.exportChannel(1)),nullptr);}
    juce::ValueTree albumState("ALBUMS");for(auto& a:albumSnapshot){juce::ValueTree side("SIDE");side.setProperty("left",packFloats(a.audio.exportChannel(0)),nullptr);side.setProperty("right",packFloats(a.audio.exportChannel(1)),nullptr);side.setProperty("rate",a.rate,nullptr);albumState.addChild(side,-1,nullptr);}root.addChild(albumState,-1,nullptr);
    return root;
}
void FieldProcessor::restoreState(const juce::ValueTree& root){
    if(!root.hasType("FIELD_STUDIO")||(int)root["version"]!=1)throw std::runtime_error("Not a supported Field Studio project");
    double sr=(double)root["sampleRate"];if(!std::isfinite(sr)||sr<8000||sr>192000)throw std::runtime_error("Invalid sample rate");
    int active=juce::jlimit(0,7,(int)root["tape"]);std::array<TapeState,8> restored;
    auto sample=root.getChildWithName("SAMPLE");double sampleSr=(double)sample["rate"];
    if(!std::isfinite(sampleSr)||sampleSr<8000||sampleSr>384000)throw std::runtime_error("Invalid sample rate");
    auto sl=unpackFloats(sample["left"],(size_t)(sampleSr*120)),rr=unpackFloats(sample["right"],(size_t)(sampleSr*120));
    if(sl.size()!=rr.size())throw std::runtime_error("Sample channels do not match");
    int count=0;
    for(auto tape:root){if(!tape.hasType("TAPE"))continue;if(count>=8||tape.getNumChildren()!=4)throw std::runtime_error("Invalid tape bank");int number=(int)tape["number"];if(number!=count)throw std::runtime_error("Invalid tape order");auto& dest=restored[(size_t)count++];dest.position=(double)tape["position"];
        for(int j=0;j<4;++j){auto track=tape.getChild(j);auto& t=dest.tracks[(size_t)j];t.audio.import(unpackFloats(track["left"],(size_t)(sr*360)),unpackFloats(track["right"],(size_t)(sr*360)));t.level=juce::jlimit(0.f,2.f,(float)track["level"]);t.pan=juce::jlimit(-1.f,1.f,(float)track["pan"]);t.mute=(bool)track["mute"];t.solo=(bool)track["solo"];auto cuts=track["cuts"];if(auto* a=cuts.getArray())for(auto& c:*a)t.cuts.push_back((double)c);}
    }
    std::array<AlbumSide,2> loadedAlbums;auto albumState=root.getChildWithName("ALBUMS");if(albumState.isValid()){if(albumState.getNumChildren()!=2)throw std::runtime_error("Invalid album sides");for(int i=0;i<2;++i){auto side=albumState.getChild(i);auto& a=loadedAlbums[(size_t)i];a.rate=(double)side["rate"];if(!std::isfinite(a.rate)||a.rate<8000||a.rate>192000)throw std::runtime_error("Invalid album sample rate");a.audio.import(unpackFloats(side["left"],(size_t)(a.rate*360)),unpackFloats(side["right"],(size_t)(a.rate*360)));}}
    if(count!=8||!root.getChildWithName("PARAMETERS").isValid())throw std::runtime_error("Incomplete project");
    std::array<float,P_count> restoredParameters{};for(int key=0;key<P_count;++key)restoredParameters[(size_t)key]=rawTargets[(size_t)key]->convertFrom0to1(rawTargets[(size_t)key]->getDefaultValue());
    for(auto item:root.getChildWithName("PARAMETERS")){const auto id=item["id"].toString();for(int key=0;key<P_count;++key)if(parameterIds[(size_t)key]==id){const float incoming=(float)item["value"];if(!std::isfinite(incoming))throw std::runtime_error("Invalid nonfinite parameter");restoredParameters[(size_t)key]=incoming;break;}}
    const auto mappingState=root.getChildWithName("MIDI_MAPPINGS");std::array<field::MidiBinding,P_count+extraMidiTargets> restoredMappings;
    if(mappingState.isValid()){juce::String mappingError;if(!parseMidiMappings(mappingState,restoredMappings,mappingError))throw std::runtime_error(mappingError.toStdString());}
    const juce::ScopedLock lock(engineLock);sampling=false;captureAudio=field::TapeAudio{};captureExhausted=false;engine.setRecording(false);engine.setPlaying(false);engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();midiClockFollower.reset();midiClockGenerator.reset();midiPositionValid=false;controllerValues.fill(-1);
    // Restore at the source rate, then prepare resamples tape for the current host.
    double targetSr=getSampleRate()>0?getSampleRate():engine.sampleRate();
    engine.tracks()={};engine.prepare(sr,juce::jmax(512,getBlockSize()));tapes=std::move(restored);albums=std::move(loadedAlbums);albumRecording=-1;albumPlaying=-1;albumHead=0;currentTape=active;currentProgram=juce::jlimit(0,7,(int)root["program"]);engine.tracks()=std::move(tapes[(size_t)active].tracks);engine.restoredTape();
    engine.prepare(targetSr,juce::jmax(512,getBlockSize()));
    if(sr!=targetSr)for(int i=0;i<8;++i)if(i!=active)for(auto& t:tapes[(size_t)i].tracks)t.audio.resample(targetSr/sr,(size_t)(targetSr*360));
    engine.selectTrack((int)root["track"]);engine.seek((double)root["position"]);selectionStart=juce::jlimit(0.,359.9,(double)root["in"]);selectionEnd=juce::jlimit(selectionStart+.1,360.,(double)root["out"]);engine.setLoop(selectionStart,selectionEnd);
    midiLearn.clearAll();for(size_t i=0;i<restoredMappings.size();++i)if(restoredMappings[i].cc>=0)midiLearn.assign(i,restoredMappings[i]);
    parameters.replaceState(root.getChildWithName("PARAMETERS"));for(int key=0;key<P_count;++key){auto* p=rawTargets[(size_t)key];p->setValue(p->convertTo0to1(restoredParameters[(size_t)key]));const float actual=cached(key);if(std::abs(actual-rawValues[(size_t)key]->load())>1e-6f)realtimeValues.fromAudio((size_t)key,actual);else realtimeValues.fromControl((size_t)key,actual);}importedPresetMetadata=juce::JSON::parse(root["presetMetadata"].toString());lastImportSummary=root["importSummary"].toString();lastImportDetails=root["importDetails"].toString();sampleLeft=std::move(sl);sampleRight=std::move(rr);sampleRateFile=sampleSr;loadLegacySample(sampleLeft,sampleRight,sampleRateFile);sampleName=sample["name"].toString();projectName=root["name"].toString();drumMode=(bool)root["drum"];hostSync=(bool)root["hostSync"];metronome=(bool)root["metronome"];inputMonitor=(bool)root["monitorInput"];hostPositionValid=false;tapeEditMask=juce::jlimit(0,15,(int)root["tapeEditMask"]);tapeSnap=(bool)root["tapeSnap"];
    auto steps=root["steps"];if(auto* a=steps.getArray())for(int i=0;i<std::min(999,a->size());++i)engine.setSequenceStep(i,(int)(*a)[i]);engine.setSequenceLength(root.hasProperty("sequenceLength")?(int)root["sequenceLength"]:16);
    auto sequencers=root.getChildWithName("SEQUENCERS");if(sequencers.isValid()){engine.setSketchLength((int)sequencers["sketchLength"]);auto sketch=sequencers["sketch"];if(auto* points=sketch.getArray())for(int i=0;i<std::min(256,points->size());++i)if(auto* p=(*points)[i].getArray())if(p->size()==2)engine.setSketchPoint(i,(float)(*p)[0],(float)(*p)[1]);auto finger=sequencers["finger"];if(auto* banks=finger.getArray())for(int bank=0;bank<std::min(16,banks->size());++bank)if(auto* pattern=(*banks)[bank].getArray())for(int step=0;step<std::min(32,pattern->size());++step)if(auto* data=(*pattern)[step].getArray())if(data->size()==2)engine.setFingerStep(bank,step,(int)(*data)[0],(float)(*data)[1]);engine.setFingerPattern((int)sequencers["fingerPattern"]);engine.setFingerLength(engine.fingerPattern(),(int)sequencers["fingerLength"]);auto lengths=sequencers["fingerLengths"];if(auto* values=lengths.getArray())for(int i=0;i<std::min(16,values->size());++i)engine.setFingerLength(i,(int)(*values)[i]);}
    auto memories=root.getChildWithName("MEMORIES");for(int i=0;i<2;++i)memos[(size_t)i]=memories.isValid()&&i<memories.getNumChildren()?memories.getChild(i).getChild(0).createCopy():juce::ValueTree{};
    auto slots=root.getChildWithName("SLOTS");for(int i=0;i<16;++i)patchSlots[(size_t)i]=slots.isValid()&&i<slots.getNumChildren()?slots.getChild(i).getChild(0).createCopy():juce::ValueTree{};
    engine.resetDboxPads();restoreDbox(engine,root["dbox"]);engine.resetDrumPads();auto pads=root.getChildWithName("PADS");if(pads.isValid()){for(int i=0;i<std::min(24,pads.getNumChildren());++i){auto node=pads.getChild(i);field::DrumPad pad;pad.start=(float)node["start"];pad.end=(float)node["end"];pad.tune=(float)node["tune"];pad.level=(float)node["level"];pad.pan=(float)node["pan"];pad.reverse=(bool)node["reverse"];pad.mode=(int)node["mode"];pad.chokeGroup=(int)node["choke"];engine.setDrumPad(i,pad);}engine.setSelectedPad((int)pads["selected"]);}updateEngine();
}
void FieldProcessor::getStateInformation(juce::MemoryBlock& out){auto state=makeState();out.reset();juce::MemoryOutputStream stream(out,false);{juce::GZIPCompressorOutputStream compressed(stream,3);state.writeToStream(compressed);}}
void FieldProcessor::setStateInformation(const void* data,int bytes){try{if(bytes<=0||bytes>512*1024*1024)return;juce::MemoryInputStream input(data,(size_t)bytes,false);auto state=readCompressedState(input);restoreState(state);}catch(const std::exception&){/* Invalid host state leaves the live project unchanged. */}}
bool FieldProcessor::saveProject(const juce::File& file,juce::String& error){juce::MemoryBlock data;getStateInformation(data);juce::TemporaryFile temp(file);if(!temp.getFile().replaceWithData(data.getData(),data.getSize())||!temp.overwriteTargetFileWithTemporary()){error="Could not save project.";return false;}projectName=file.getFileNameWithoutExtension();return true;}
bool FieldProcessor::loadProject(const juce::File& file,juce::String& error){if(file.getSize()>512ll*1024*1024){error="Project exceeds the 512 MB import limit.";return false;}juce::FileInputStream input(file);if(!input.openedOk()){error="Could not open project.";return false;}try{restoreState(readCompressedState(input));projectName=file.getFileNameWithoutExtension();return true;}catch(const std::exception& e){error=e.what();return false;}}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new FieldProcessor();}
