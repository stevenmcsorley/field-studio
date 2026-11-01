#include "PluginProcessor.h"
namespace {
const char* extraIds[]{"track1Level","track1Pan","track1Mute","track1Solo","track2Level","track2Pan","track2Mute","track2Solo","track3Level","track3Pan","track3Mute","track3Solo","track4Level","track4Pan","track4Mute","track4Solo","padStart","padEnd","padTune","padLevel","padPan","padMode","padChoke","dboxMix","dboxWave1","dboxEnvelope1","dboxCrossMod","dboxFrequency","dboxWave2","dboxEnvelope2","dboxCutoff","dboxAttack","dboxGain","dboxRelease","dboxSmooth","tapePlay","tapeStop","tapeRewind","tapeSeek","loopStart","loopEnd","sequenceLength","tapeRecord","tapeCopy","tapeLift","tapeDrop","tapeSplit","tapeJoin","tapeMerge","tapeBounce","tapeClear","tapeUndo","tapeRedo","tapeSlideBack","tapeSlideForward","sampleCaptureInput","sampleCaptureOutput","sampleCaptureStop","tapeTrack","tapeBack","tapeForward","tapeBarBack","tapeBarForward","tapeEnd","tapeSetLoopIn","tapeSetLoopOut","tapeLoopToggle","tapeBrake","tapeReverse","tapeRoll","tapeJog","metronomeOn"};
static_assert(std::size(extraIds)==72);
const char* extraLabels[]{"Track 1 level","Track 1 pan","Track 1 mute","Track 1 solo","Track 2 level","Track 2 pan","Track 2 mute","Track 2 solo","Track 3 level","Track 3 pan","Track 3 mute","Track 3 solo","Track 4 level","Track 4 pan","Track 4 mute","Track 4 solo","Selected pad start","Selected pad end","Selected pad tuning","Selected pad level","Selected pad pan","Selected pad mode","Selected pad choke","DBox mix","DBox waveform 1","DBox envelope 1","DBox cross modulation","DBox frequency","DBox waveform 2","DBox envelope 2","DBox cutoff","DBox attack","DBox gain","DBox release","DBox smoothing","Tape play","Tape stop","Tape rewind","Tape seek","Loop start","Loop end","Sequence length","Tape record / finish recording","Tape copy","Tape lift","Tape drop","Tape split","Tape join","Tape merge","Tape bounce","Tape clear selection","Tape undo","Tape redo","Tape slide backward","Tape slide forward","Sample capture input","Sample capture output","Sample capture finish","Tape selected track","Tape backward one second","Tape forward one second","Tape previous bar","Tape next bar","Tape recorded end","Set loop in at playhead","Set loop out at playhead","Toggle tape loop","Tape brake (hold)","Tape reverse (hold)","Tape roll (hold)","Tape fine jog (relative encoder)","Metronome enabled"};
const float dboxLow[]{0,0,.005f,0,0,0,.005f,0,.0002f,0,.005f,0};
const float dboxHigh[]{1,1,4,1,1,1,4,1,1,2,4,1};
std::array<float*,12> dboxFields(field::DBoxPad& p){return {&p.mix,&p.wave1,&p.envelope1,&p.crossMod,&p.frequency,&p.wave2,&p.envelope2,&p.cutoff,&p.attack,&p.gain,&p.release,&p.smooth};}
}
int FieldProcessor::midiMappingIndex(const juce::String& id) const {for(int i=0;i<P_count;++i)if(parameterIds[(size_t)i]==id)return i;for(size_t i=0;i<extraMidiTargets;++i)if(id==extraIds[i])return P_count+(int)i;return -1;}
juce::StringArray FieldProcessor::midiMappingTargets() const {juce::StringArray ids;for(auto& id:parameterIds)ids.add(id);for(auto id:extraIds)ids.add(id);return ids;}
juce::String FieldProcessor::midiMappingLabel(const juce::String& id) const {int target=midiMappingIndex(id);return target<0?juce::String{}:target<P_count?rawTargets[(size_t)target]->getName(80):juce::String(extraLabels[target-P_count]);}
juce::String FieldProcessor::midiMappingDescription(const juce::String& id) const {int target=midiMappingIndex(id);if(target<0)return {};const auto b=midiLearn.binding((size_t)target);if(b.cc<0)return "Unassigned";const juce::StringArray modes{"absolute","relative two's complement","relative offset 64","relative sign/magnitude"};return "CC "+juce::String(b.cc)+" / "+(b.channel?"channel "+juce::String(b.channel):"omni")+" / "+modes[(int)b.mode]+(b.mode==field::EncoderMode::absolute&&b.pickup?" / pickup":"")+(b.invert?" / reversed":"");}
juce::String FieldProcessor::midiLearnTarget() const {int target=midiLearn.learningTarget();return target<0?juce::String{}:target<P_count?parameterIds[(size_t)target]:juce::String(extraIds[target-P_count]);}
bool FieldProcessor::beginMidiLearn(const juce::String& id,field::EncoderMode mode,bool pickup,bool invert){const int target=midiMappingIndex(id);return target>=0&&midiLearn.arm((size_t)target,mode,((target>=P_count+35&&target<=P_count+37)||(target>=P_count+42&&target!=P_count+58))?false:pickup,invert);}
void FieldProcessor::clearMidiMapping(const juce::String& id){const int target=midiMappingIndex(id);if(target>=0)midiLearn.clear((size_t)target);}
juce::ValueTree FieldProcessor::midiMappingState() const {juce::ValueTree root("MIDI_MAPPINGS");root.setProperty("version",1,nullptr);auto ids=midiMappingTargets();for(int target=0;target<ids.size();++target){auto b=midiLearn.binding((size_t)target);if(b.cc<0)continue;juce::ValueTree item("BINDING");item.setProperty("target",ids[target],nullptr);item.setProperty("channel",b.channel,nullptr);item.setProperty("cc",b.cc,nullptr);item.setProperty("mode",(int)b.mode,nullptr);item.setProperty("pickup",b.pickup,nullptr);item.setProperty("invert",b.invert,nullptr);root.addChild(item,-1,nullptr);}return root;}
bool FieldProcessor::parseMidiMappings(const juce::ValueTree& root,std::array<field::MidiBinding,P_count+extraMidiTargets>& bindings,juce::String& error) const {
    constexpr size_t count=P_count+extraMidiTargets;std::array<bool,count> seen{};
    if(!root.hasType("MIDI_MAPPINGS")||(int)root["version"]!=1||root.getNumChildren()>(int)count){error="Unsupported MIDI mapping file.";return false;}
    for(auto item:root){for(const char* field:{"channel","cc","mode"}){const auto text=item[field].toString();if(text.isEmpty()||!text.containsOnly("0123456789")||text.length()>3){error="Invalid numeric MIDI binding field; existing assignments were retained.";return false;}}const int target=midiMappingIndex(item["target"].toString());field::MidiBinding b{(int)item["channel"],(int)item["cc"],(field::EncoderMode)(int)item["mode"],(bool)item["pickup"],(bool)item["invert"]};if(!item.hasType("BINDING")||target<0||seen[(size_t)target]||!item.hasProperty("cc")||!item.hasProperty("channel")||!item.hasProperty("mode")||!decltype(midiLearn)::valid(b)){error="Invalid MIDI mapping; existing assignments were retained.";return false;}seen[(size_t)target]=true;bindings[(size_t)target]=b;}
    return true;
}
bool FieldProcessor::restoreMidiMappingState(const juce::ValueTree& root,juce::String& error){std::array<field::MidiBinding,P_count+extraMidiTargets> bindings;if(!parseMidiMappings(root,bindings,error))return false;midiLearn.clearAll();for(size_t i=0;i<bindings.size();++i)if(bindings[i].cc>=0)midiLearn.assign(i,bindings[i]);return true;
}
float FieldProcessor::mappedMidiValue(size_t target) const noexcept {
    const auto& engine=renderingEngine();const double selectionStart=offlineFixture?this->selectionStart:renderDocument.loopStart,selectionEnd=offlineFixture?this->selectionEnd:renderDocument.loopEnd;
    if(target<P_count)return rawTargets[target]->getValue();size_t key=target-P_count;
    if(key<16){int track=(int)key/4;switch(key%4){case 0:return engine.trackLevel(track)/2;case 1:return (engine.trackPan(track)+1)/2;case 2:return engine.trackMute(track)?1:0;default:return engine.trackSolo(track)?1:0;}}
    if(key<23){const auto& p=engine.drumPad(engine.selectedPad());switch(key-16){case 0:return p.start;case 1:return p.end;case 2:return (p.tune+24)/48;case 3:return p.level/2;case 4:return (p.pan+1)/2;case 5:return p.mode/2.f;default:return p.chokeGroup/4.f;}}
    if(key<35){auto p=engine.dboxPad(engine.selectedPad());const auto i=key-23;return (*dboxFields(p)[i]-dboxLow[i])/(dboxHigh[i]-dboxLow[i]);}
    if(key==41){int maximum=(int)value("sequencer")==3?32:999;int length=(int)value("sequencer")==3?engine.fingerLength(engine.fingerPattern()):engine.sequenceLength();return (length-1.f)/(maximum-1);}
    if(key==70)return .5f;if(key==71)return metronome?1:0;
    if(key>=42){if(key==58)return engine.selectedTrack()/3.f;return 0;}
    switch(key){case 35:return engine.playing()?1:0;case 36:return engine.playing()?0:1;case 37:return 0;case 38:return (float)(engine.positionSeconds()/360);case 39:return (float)(selectionStart/360);default:return (float)(selectionEnd/360);}
}
void FieldProcessor::applyMappedMidi(size_t target,float value) noexcept {
    auto& engine=renderingEngine();auto& selectionStart=offlineFixture?this->selectionStart:renderDocument.loopStart;auto& selectionEnd=offlineFixture?this->selectionEnd:renderDocument.loopEnd;
    value=juce::jlimit(0.f,1.f,value);if(target<P_count){setNormalized((int)target,value);return;}const size_t key=target-P_count;
    if(key<16){int track=(int)key/4;switch(key%4){case 0:engine.setTrackLevel(track,value*2);break;case 1:engine.setTrackPan(track,value*2-1);break;case 2:engine.setTrackMute(track,value>=.5f);break;default:engine.setTrackSolo(track,value>=.5f);}return;}
    if(key<23){int selected=engine.selectedPad();auto p=engine.drumPad(selected);switch(key-16){case 0:p.start=value;break;case 1:p.end=value;break;case 2:p.tune=value*48-24;break;case 3:p.level=value*2;break;case 4:p.pan=value*2-1;break;case 5:p.mode=(int)std::lround(value*2);break;default:p.chokeGroup=(int)std::lround(value*4);}engine.setDrumPad(selected,p);return;}
    if(key<35){int selected=engine.selectedPad();auto p=engine.dboxPad(selected);const auto i=key-23;*dboxFields(p)[i]=dboxLow[i]+value*(dboxHigh[i]-dboxLow[i]);engine.setDboxPad(selected,p);return;}
    if(key==41){int maximum=(int)this->value("sequencer")==3?32:999;int length=1+(int)std::lround(value*(maximum-1));if(maximum==32)engine.setFingerLength(engine.fingerPattern(),length);else engine.setSequenceLength(length);return;}
    if(key==70){engine.seek(engine.positionSeconds()+(value-.5f)*2);midiLearn.rebaseRelative(target,.5f);return;}
    if(key==71){metronome=value>=.5f;return;}
    if(key>=42){
        // Action destinations are rising-edge buttons; releases re-arm them.
        if(key==58){engine.setRecording(false);engine.selectTrack((int)std::lround(value*3));return;}
        if(key>=67){if(key==67)engine.setTapeBrake(value>=.5f);else if(key==68)engine.setTapeReverse(value>=.5f);else engine.setTapeRoll(value>=.5f,.25f);return;}
        const bool down=value>=.5f;const bool pressed=down&&!midiActionDown[key-42];midiActionDown[key-42]=down;if(!pressed)return;
        if(key<=57){if(!midiTapeActions.push({(int)key,midiActionEpoch.load(std::memory_order_acquire)})){++rejectedLiveCommands;midiActionOverflow=true;}return;}
        const double bar=240./std::max(30.f,engine.parameters().tempo);
        if(key==59||key==60)engine.seek(engine.positionSeconds()+(key==59?-1:1));
        else if(key==61||key==62)engine.seek(engine.positionSeconds()+(key==61?-bar:bar));
        else if(key==63)engine.seek(engine.recordedSeconds());
        else if(key==64){selectionStart=juce::jlimit(0.,359.9,engine.positionSeconds());selectionEnd=std::max(selectionEnd,selectionStart+.1);engine.setLoop(selectionStart,selectionEnd);}
        else if(key==65){selectionEnd=juce::jlimit(selectionStart+.1,360.,engine.positionSeconds());engine.setLoop(selectionStart,selectionEnd);}
        else if(key==66)setPhysical(P_loop,cached(P_loop)>.5f?0.f:1.f);
        return;
    }
    switch(key){case 35:if(value>=.5f)engine.setPlaying(true);break;case 36:if(value>=.5f){++controlRevision;midiActionEpoch.fetch_add(1,std::memory_order_release);midiStopRequested=true;engine.setRecording(false);engine.setPlaying(false);engine.setTapeBrake(false);engine.setTapeReverse(false);engine.setTapeRoll(false);}break;case 37:if(value>=.5f)engine.rewind();break;case 38:engine.seek(value*359.999);break;case 39:selectionStart=juce::jlimit(0.,selectionEnd-.1,value*360.);engine.setLoop(selectionStart,selectionEnd);break;case 40:selectionEnd=juce::jlimit(selectionStart+.1,360.,value*360.);engine.setLoop(selectionStart,selectionEnd);break;}
}
