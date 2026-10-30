#include "MidiSupport.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace field::midi {
namespace {
double finiteRate(double rate){return std::clamp(std::isfinite(rate)?rate:44100.0,8000.0,192000.0);}
double finiteTempo(double bpm){return std::clamp(std::isfinite(bpm)?bpm:120.0,20.0,400.0);}
}
void ClockFollower::prepare(double sampleRate){sr=finiteRate(sampleRate);reset();}
void ClockFollower::clearEstimate(){intervals.fill(0);intervalCount=writeIndex=candidateCount=0;candidate=0;estimatedBpm=120;}
void ClockFollower::reset(){clearEstimate();havePulse=false;lastPulse=0;running=false;firstStartPulse=false;clockPosition=0;}
void ClockFollower::advance(int64_t sample){
    if(havePulse && sample>=lastPulse && static_cast<double>(sample-lastPulse)>sr*2){clearEstimate();havePulse=false;}
}
void ClockFollower::accept(double interval){
    intervals[static_cast<size_t>(writeIndex)]=interval;writeIndex=(writeIndex+1)%static_cast<int>(intervals.size());
    intervalCount=std::min(intervalCount+1,static_cast<int>(intervals.size()));
    auto sorted=intervals;std::sort(sorted.begin(),sorted.begin()+intervalCount);
    const double median=intervalCount%2?sorted[static_cast<size_t>(intervalCount/2)]:(sorted[static_cast<size_t>(intervalCount/2-1)]+sorted[static_cast<size_t>(intervalCount/2)])*.5;
    estimatedBpm=60.0*sr/(24.0*median);
}
void ClockFollower::pulse(int64_t sample){
    if(sample<0)return;
    advance(sample);
    if(havePulse && sample<=lastPulse){if(sample==lastPulse)return;clearEstimate();havePulse=false;}
    if(running){if(firstStartPulse)firstStartPulse=false;else clockPosition+=1;}
    if(!havePulse){lastPulse=sample;havePulse=true;return;}
    const double interval=static_cast<double>(sample-lastPulse);lastPulse=sample;
    if(interval<sr*60/(24*400.0) || interval>sr*60/(24*20.0))return;
    const double reference=sr*60/(24*estimatedBpm);
    if(intervalCount>=3 && std::abs(interval/reference-1)>.15){
        if(candidateCount>0 && std::abs(interval/candidate-1)<.08){candidate=(candidate*candidateCount+interval)/(candidateCount+1);++candidateCount;}
        else{candidate=interval;candidateCount=1;}
        if(candidateCount>=3){const double changed=candidate;clearEstimate();for(int i=0;i<3;++i)accept(changed);}
        return;
    }
    candidateCount=0;candidate=0;accept(interval);
}
bool ClockFollower::observe(const juce::MidiMessage& message,int64_t sample){
    if(message.isMidiClock()){pulse(sample);return true;}
    if(message.isMidiStart()){running=true;clockPosition=0;firstStartPulse=true;return true;}
    if(message.isMidiContinue()){running=true;firstStartPulse=false;return true;}
    if(message.isMidiStop()){running=false;return true;}
    if(message.isSongPositionPointer()){clockPosition=message.getSongPositionPointerMidiBeat()*6.0;firstStartPulse=false;return true;}
    return false;
}
int ClockFollower::songPosition()const{return std::clamp(static_cast<int>(clockPosition/6),0,16383);}

void ClockGenerator::prepare(double sampleRate){sr=finiteRate(sampleRate);reset();}
void ClockGenerator::reset(){tempo=120;expectedPosition=nextClock=0;cursor=nextPulse=0;running=havePosition=false;}
void ClockGenerator::position(juce::MidiBuffer& output,double ppq,int offset,bool sendStop){
    if(sendStop)output.addEvent(juce::MidiMessage::midiStop(),offset);
    if(ppq<.000001)output.addEvent(juce::MidiMessage::midiStart(),offset);
    else{output.addEvent(juce::MidiMessage::songPositionPointer(std::clamp(static_cast<int>(std::min(16383.0,std::floor(ppq*4))),0,16383)),offset);output.addEvent(juce::MidiMessage::midiContinue(),offset);}
    nextPulse=static_cast<int64_t>(std::ceil(ppq*24-1e-8));
    nextClock=static_cast<double>(cursor)+(nextPulse/24.0-ppq)*sr*60/tempo;
}
void ClockGenerator::process(juce::MidiBuffer& output,int frames,double requestedTempo,bool enabled,bool play,double ppq,bool suppress,int offset){
    if(frames<=0)return;
    offset=std::clamp(offset,0,std::numeric_limits<int>::max()-frames);
    ppq=std::clamp(std::isfinite(ppq)?ppq:expectedPosition,0.0,1.0e9);
    requestedTempo=finiteTempo(requestedTempo);
    if(suppress){running=havePosition=false;cursor+=frames;return;}
    if(!enabled){if(running)output.addEvent(juce::MidiMessage::midiStop(),offset);running=havePosition=false;cursor+=frames;return;}
    if(!play){if(running)output.addEvent(juce::MidiMessage::midiStop(),offset);running=false;havePosition=true;expectedPosition=ppq;cursor+=frames;return;}
    const bool seek=havePosition && std::abs(ppq-expectedPosition)>1.0/96.0;
    if(!running || seek){tempo=requestedTempo;position(output,ppq,offset,running);running=true;}
    else if(std::abs(tempo-requestedTempo)>1e-9){tempo=requestedTempo;nextClock=static_cast<double>(cursor)+(nextPulse/24.0-ppq)*sr*60/tempo;}
    const double spacing=sr*60/(tempo*24);
    while(nextClock<static_cast<double>(cursor)+frames+.5){
        const auto at=static_cast<int64_t>(std::llround(nextClock-static_cast<double>(cursor)));
        if(at>=frames)break;
        output.addEvent(juce::MidiMessage::midiClock(),offset+static_cast<int>(std::max<int64_t>(0,at)));
        ++nextPulse;nextClock+=spacing;
    }
    expectedPosition=ppq+frames*tempo/(sr*60);havePosition=true;cursor+=frames;
}

bool NoteState::valid(int channel,int note){return channel>=1&&channel<=16&&note>=0&&note<128;}
bool NoteState::heldAnywhere(int note)const{
    if(note<0||note>=128)return false;
    for(size_t channel=0;channel<16;++channel)if(keys[channel][static_cast<size_t>(note)]||sustained[channel][static_cast<size_t>(note)])return true;
    return false;
}
bool NoteState::held(int channel,int note)const{return valid(channel,note)&&(keys[static_cast<size_t>(channel-1)][static_cast<size_t>(note)]||sustained[static_cast<size_t>(channel-1)][static_cast<size_t>(note)]);}
bool NoteState::pedalDown(int channel)const{return channel>=1&&channel<=16&&pedals[static_cast<size_t>(channel-1)];}
bool NoteState::noteOn(int channel,int note){
    if(!valid(channel,note))return false;
    keys[static_cast<size_t>(channel-1)][static_cast<size_t>(note)]=true;sustained[static_cast<size_t>(channel-1)][static_cast<size_t>(note)]=false;return true;
}
bool NoteState::noteOff(int channel,int note){
    if(!valid(channel,note))return false;
    const auto c=static_cast<size_t>(channel-1),n=static_cast<size_t>(note);
    const bool owned=keys[c][n]||sustained[c][n];
    const bool wasKey=keys[c][n];keys[c][n]=false;
    if(pedals[c]&&wasKey)sustained[c][n]=true;
    else if(!pedals[c])sustained[c][n]=false;
    return owned&&!heldAnywhere(note);
}
Releases NoteState::controllerSustain(int channel,bool down){
    Releases result;if(channel<1||channel>16)return result;const auto c=static_cast<size_t>(channel-1);
    pedals[c]=down;if(down)return result;
    for(int note=0;note<128;++note)if(sustained[c][static_cast<size_t>(note)]){
        sustained[c][static_cast<size_t>(note)]=false;if(!heldAnywhere(note))result.notes[static_cast<size_t>(result.count++)]=note;
    }
    return result;
}
Releases NoteState::resetChannel(int channel){
    Releases result;if(channel<1||channel>16)return result;const auto c=static_cast<size_t>(channel-1);pedals[c]=false;
    for(int note=0;note<128;++note){const auto n=static_cast<size_t>(note);const bool owned=keys[c][n]||sustained[c][n];keys[c][n]=sustained[c][n]=false;if(owned&&!heldAnywhere(note))result.notes[static_cast<size_t>(result.count++)]=note;}
    return result;
}
Releases NoteState::resetAll(){
    Releases result;for(int note=0;note<128;++note)if(heldAnywhere(note))result.notes[static_cast<size_t>(result.count++)]=note;
    for(auto& channel:keys)channel.fill(false);
    for(auto& channel:sustained)channel.fill(false);
    pedals.fill(false);return result;
}
}
