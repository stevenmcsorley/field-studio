#include "MidiSupport.h"
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>
namespace {std::atomic<bool> watch{false};std::atomic<size_t> allocations{0};}
void* operator new(std::size_t n){if(watch.load())++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
using namespace field::midi;
namespace {
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
template<class F>void noAlloc(F f){auto before=allocations.load();watch=true;f();watch=false;check(allocations.load()==before,"MIDI realtime operation allocates no heap memory");}
int count(const juce::MidiBuffer& buffer,int status){int result=0;for(auto event:buffer)if(event.getMessage().getRawData()[0]==status)++result;return result;}
}
int main(){
    {
        NoteState notes;noAlloc([&]{check(notes.noteOn(1,60)&&notes.noteOn(2,60),"multi-channel note-ons retrigger");check(!notes.noteOff(1,60)&&notes.heldAnywhere(60),"one channel cannot release another channel's held pitch");check(notes.noteOff(2,60)&&!notes.heldAnywhere(60),"last owning channel releases shared pitch");
        notes.noteOn(1,60);notes.controllerSustain(1,true);check(!notes.noteOff(1,60),"sustain defers channel note-off");notes.noteOn(2,60);check(notes.controllerSustain(1,false).count==0,"pedal release respects other channels");check(notes.noteOff(2,60),"final physical key releases after other pedal is up");
        notes.noteOn(1,61);notes.noteOn(2,62);notes.controllerSustain(1,true);notes.controllerSustain(2,true);notes.noteOff(1,61);notes.noteOff(2,62);auto release=notes.controllerSustain(1,false);check(release.count==1&&release.notes[0]==61&&notes.held(2,62),"pedals are independent per channel");
        notes.noteOn(1,62);check(notes.resetChannel(2).count==0,"channel reset preserves shared notes");auto all=notes.resetAll();check(all.count==1&&all.notes[0]==62&&!notes.heldAnywhere(62),"resetAll emits one off for each unique owned pitch");check(!notes.noteOn(0,60)&&!notes.noteOff(17,60)&&notes.resetChannel(99).count==0,"invalid note channels are ignored");});
    }
    {
        ClockFollower clock;clock.prepare(48000);noAlloc([&]{clock.observe(juce::MidiMessage::midiStart(),0);for(int tick=0;tick<30;++tick)clock.observe(juce::MidiMessage::midiClock(),tick*1000);});check(clock.ready()&&std::abs(clock.bpm()-120)<.0001,"24PPQN follower measures120BPM");check(clock.playing()&&std::abs(clock.quarterNotes()-29/24.0)<1e-9,"start clock position advances from first pulse");
        noAlloc([&]{clock.observe(juce::MidiMessage::midiClock(),31000);clock.observe(juce::MidiMessage::midiClock(),32000);});check(std::abs(clock.bpm()-120)<.01,"one missing pulse does not halve tempo");
        noAlloc([&]{for(int tick=1;tick<=12;++tick)clock.observe(juce::MidiMessage::midiClock(),32000+tick*2000);});check(clock.ready()&&std::abs(clock.bpm()-60)<.01,"consistent new clock tempo replaces old estimate");
        noAlloc([&]{clock.observe(juce::MidiMessage::midiStop(),56000);clock.observe(juce::MidiMessage::songPositionPointer(32),56000);clock.observe(juce::MidiMessage::midiContinue(),56000);});check(clock.playing()&&clock.songPosition()==32&&clock.quarterNotes()==8,"SPP and continue restore musical position");clock.advance(200000);check(!clock.ready(),"stale incoming clock estimate times out");
        clock.prepare(44100);noAlloc([&]{for(int tick=0;tick<50;++tick)clock.observe(juce::MidiMessage::midiClock(),static_cast<int64_t>(std::llround(tick*44100.0*60/(137*24)))+(tick%3-1));});check(clock.ready()&&std::abs(clock.bpm()-137)<.08,"median follower rejects sample timestamp jitter");
    }
    {
        ClockGenerator generator;generator.prepare(48000);juce::MidiBuffer buffer;buffer.ensureSize(65536);std::vector<int64_t> ticks;ticks.reserve(100);int starts=0;
        for(int64_t at=0;at<48000;){int n=static_cast<int>(std::min<int64_t>(512,48000-at));buffer.clear();noAlloc([&]{generator.process(buffer,n,120,true,true,at/24000.0);});starts+=count(buffer,0xfa);for(auto event:buffer)if(event.getMessage().isMidiClock())ticks.push_back(at+event.samplePosition);at+=n;}
        check(starts==1&&ticks.size()==48,"generator sends one start and24 clocks per quarter note");for(size_t i=0;i<ticks.size();++i)check(ticks[i]==static_cast<int64_t>(i)*1000,"clock timing is sample-accurate across block boundaries");
        buffer.clear();noAlloc([&]{generator.process(buffer,512,120,true,false,2);});check(count(buffer,0xfc)==1&&count(buffer,0xf8)==0,"stopping sends stop and no clock pulses");buffer.clear();noAlloc([&]{generator.process(buffer,512,120,true,true,2);});check(count(buffer,0xfb)==1&&count(buffer,0xf2)==1,"resuming away from zero sends SPP and continue");
        buffer.clear();noAlloc([&]{generator.process(buffer,512,120,true,true,8);});check(count(buffer,0xfc)==1&&count(buffer,0xf2)==1&&count(buffer,0xfb)==1,"running seek repositions downstream transport");for(auto event:buffer)if(event.getMessage().isSongPositionPointer())check(event.getMessage().getSongPositionPointerMidiBeat()==32,"seek sends correct sixteenth-note SPP");buffer.clear();noAlloc([&]{generator.process(buffer,512,120,true,true,8+512/24000.0,true);});check(buffer.isEmpty(),"external clock suppression emits no clock or transport echo");
    }
    {
        ClockGenerator generator;generator.prepare(48000);juce::MidiBuffer buffer;buffer.ensureSize(65536);noAlloc([&]{generator.process(buffer,600,120,true,true,0,false,0);generator.process(buffer,600,120,true,true,600/24000.0,false,600);});std::vector<int> times;for(auto event:buffer)if(event.getMessage().isMidiClock())times.push_back(event.samplePosition);check(times.size()==2&&times[0]==0&&times[1]==1000,"segmented render writes clock at outer block offset");
        generator.reset();buffer.clear();generator.process(buffer,1000,120,true,true,0);buffer.clear();noAlloc([&]{generator.process(buffer,1000,60,true,true,1000/24000.0);});check(count(buffer,0xfa)==0&&count(buffer,0xfb)==0&&count(buffer,0xf8)==1,"tempo change preserves transport and next pulse phase");buffer.clear();noAlloc([&]{generator.process(buffer,2000,60,true,true,.0625);});for(auto event:buffer)if(event.getMessage().isMidiClock())check(event.samplePosition==1000,"new tempo changes pulse spacing without resetting position");
    }
    {
        ClockGenerator generator;generator.prepare(44100);juce::MidiBuffer buffer;buffer.ensureSize(65536);std::vector<int64_t> ticks;ticks.reserve(100);for(int64_t at=0;at<44100;at+=441){buffer.clear();noAlloc([&]{generator.process(buffer,441,137,true,true,at*137.0/(44100*60));});for(auto event:buffer)if(event.getMessage().isMidiClock())ticks.push_back(at+event.samplePosition);}const double spacing=44100.0*60/(137*24);for(size_t i=0;i<ticks.size();++i)check(std::abs(ticks[i]-i*spacing)<=.50001,"fractional clock periods round to nearest sample without drift");
    }
    std::cout<<"PASS: MIDI channel ownership, per-channel sustain/reset, incoming clock jitter/outliers/tempo/timeout, outgoing24PPQN timing/transport/seek/suppression/segments and allocation-free helpers\n";
}
