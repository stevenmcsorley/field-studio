#include "PluginProcessor.h"
#include <cmath>

namespace {
void applyEditingCommand(field::Engine& engine,const field::LiveCommand& command){
 using C=field::LiveControl;const int a=command.a,b=command.b;const float x=(float)command.x;
        switch(command.kind){
            case C::play:engine.setPlaying(!engine.playing());break;
            case C::rewind:engine.rewind();break;
            case C::seek:engine.seek(command.x);break;
            case C::seekRelative:engine.seek(engine.positionSeconds()+command.x);break;
            case C::track:engine.setRecording(false);engine.selectTrack(a);break;
            case C::level:engine.setTrackLevel(a,x);break;
            case C::pan:engine.setTrackPan(a,x);break;
            case C::mute:engine.setTrackMute(a,!engine.trackMute(a));break;
            case C::solo:engine.setTrackSolo(a,!engine.trackSolo(a));break;
            case C::brake:engine.setTapeBrake(x>.5f);break;
            case C::reverse:engine.setTapeReverse(x>.5f);break;
            case C::roll:engine.setTapeRoll(x>.5f,.25f);break;
            case C::pad:engine.setSelectedPad(a);break;
            case C::drumParameter:{auto pad=engine.drumPad(a);switch(b){case 0:pad.start=x;break;case 1:pad.end=x;break;case 2:pad.tune=x;break;case 3:pad.level=x;break;case 4:pad.pan=x;break;case 5:pad.mode=(int)x;break;case 6:pad.chokeGroup=(int)x;break;case 7:pad.reverse=!pad.reverse;break;default:break;}engine.setDrumPad(a,pad);break;}
            case C::dboxParameter:{auto pad=engine.dboxPad(a);switch(b){case 0:pad.mix=x;break;case 1:pad.wave1=x;break;case 2:pad.envelope1=x;break;case 3:pad.crossMod=x;break;case 4:pad.frequency=x;break;case 5:pad.wave2=x;break;case 6:pad.envelope2=x;break;case 7:pad.cutoff=x;break;case 8:pad.attack=x;break;case 9:pad.gain=x;break;case 10:pad.release=x;break;case 11:pad.smooth=x;break;default:break;}engine.setDboxPad(a,pad);break;}
            case C::fingerBank:engine.setFingerPattern(a);break;
            case C::fingerLength:engine.setFingerLength(a,b);break;
            case C::sequenceLength:engine.setSequenceLength(a);break;
            case C::sequenceNote:if(a>=0&&a<999&&b>=0&&b<128)engine.setSequenceNote(a,b,x>.5f?!engine.sequenceNotes(a)[(size_t)b]:false);break;
            case C::sequenceStep:engine.setSequenceStep(a,b,x);break;
            case C::fingerStep:{const int old=engine.fingerStep(a,b);engine.setFingerStep(a,b,command.x>.5? (old<0?60:std::min(127,old+1)) :old<0&&command.y<.5?60:-1);break;}
            case C::clearHeld:engine.clearHeldNotes();break;
            case C::joinFinger:engine.clearHeldNotes();engine.noteOn(53+a/2*2,.8f);engine.noteOn(54+a/2*2,.8f);break;
            case C::manualAdvance:engine.advanceSequence();break;
            case C::sketchPoint:engine.setSketchPoint(a,x,(float)command.y);engine.setSketchLength(b);break;
        }
}
}

void FieldProcessor::invalidateLiveControls(){
    const std::lock_guard<std::mutex> lock(liveProducerMutex);
    liveEpoch.fetch_add(1,std::memory_order_release);livePerformance=8;
}

bool FieldProcessor::requestLiveControl(field::LiveControl kind,int a,int b,double x,double y){
    if(!std::isfinite(x)||!std::isfinite(y))return false;
    using C=field::LiveControl;
    if(!offlineFixture&&kind!=C::manualAdvance&&kind!=C::clearHeld&&kind!=C::joinFinger){
        if(kind==C::brake||kind==C::reverse||kind==C::roll){requestTapePerformance(kind==C::brake?0:kind==C::reverse?1:2,x>.5);return true;}
        DocumentGuard document(*this);applyEditingCommand(engine,{kind,a,b,x,y,liveEpoch.load()});++controlRevision;return true;
    }
    const std::lock_guard<std::mutex> lock(liveProducerMutex);
    if(liveCommands.push({kind,a,b,x,y,liveEpoch.load(std::memory_order_relaxed)}))return true;
    ++rejectedLiveCommands;reportOperationError("Live control queue full; retry the control. Stop and panic remain available.");return false;
}
void FieldProcessor::requestLiveStop(bool panic){
    cancelMediaJob();
    {const std::lock_guard<std::mutex> lock(liveProducerMutex);
    // Epoch fences prevent old Play or held-performance commands from undoing
    // Stop/Panic even when the ordinary queue is full.
    liveEpoch.fetch_add(1,std::memory_order_release);
    livePerformance=8;
    urgentLiveActions.fetch_or(panic?2u:1u,std::memory_order_release);
    keyboard.allNotesOff(0);}
    if(!offlineFixture){DocumentGuard document(*this);if(!panic){engine.setRecording(false);engine.setPlaying(false);documentAlbumRecording=-1;documentAlbumPlaying=-1;}controlSignals|=1u;}
}
void FieldProcessor::applyUrgentLiveActions(){
    auto& engine=renderingEngine();
    const auto actions=urgentLiveActions.exchange(0,std::memory_order_acq_rel);if(!actions)return;
    ++controlRevision;engine.allNotesOff();midiNotes.resetAll();guiMidiNotes.resetAll();
    engine.setTapeBrake(false);engine.setTapeReverse(false);engine.setTapeRoll(false);
    if(actions&1u){if(offlineFixture){albumRecording=-1;albumPlaying=-1;}else{renderDocument.albumRecording=-1;renderDocument.albumPlaying=-1;}engine.setRecording(false);engine.setPlaying(false);}
}
void FieldProcessor::applyLiveCommands(){
    auto& engine=renderingEngine();
    using C=field::LiveControl;applyUrgentLiveActions();field::LiveCommand command;
    const auto performance=livePerformance.fetch_and(7u);if(performance&8u){engine.setTapeBrake((performance&1)!=0);engine.setTapeReverse((performance&2)!=0);engine.setTapeRoll((performance&4)!=0,.25f);}
    for(int count=0;count<32&&liveCommands.pop(command);++count){
        applyUrgentLiveActions();if(command.epoch!=liveEpoch.load(std::memory_order_acquire))continue;
        const int a=command.a,b=command.b;const float x=(float)command.x;
        ++controlRevision;
        applyEditingCommand(engine,command);
    }
    applyUrgentLiveActions();
}
