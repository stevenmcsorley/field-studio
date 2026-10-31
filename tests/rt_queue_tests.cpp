#include "RtKeyboard.h"
#include "RealtimeValues.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
namespace {thread_local bool watching=false;std::atomic<size_t> allocations{0},frees{0};}
void* operator new(std::size_t n){if(watching)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{if(p&&watching)++frees;std::free(p);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,std::size_t)noexcept{::operator delete(p);}
void operator delete[](void* p,std::size_t)noexcept{::operator delete(p);}
static void check(bool ok,const char* text){if(!ok){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);}}
int main(){
    {field::RtQueue<int,8> queue;int value=-1;check(!queue.pop(value),"empty queue");for(int i=0;i<8;++i)check(queue.push(i),"fill queue");check(!queue.push(99),"backpressure");for(int i=0;i<8;++i){check(queue.pop(value)&&value==i,"FIFO preserves full payload");}for(int i=0;i<200;++i){check(queue.push(i)&&queue.pop(value)&&value==i,"indices wrap");}queue.push(1);queue.discardPending();check(!queue.pop(value),"consumer discard");}
    {struct Item{uint64_t sequence,inverse;};field::RtQueue<Item,256> queue;std::atomic<bool> intact{true};constexpr uint64_t count=500000;
    std::thread producer([&]{for(uint64_t i=0;i<count;++i)while(!queue.push({i,~i}))std::this_thread::yield();});
    for(uint64_t expected=0;expected<count;++expected){Item item{};while(!queue.pop(item))std::this_thread::yield();if(item.sequence!=expected||item.inverse!=~expected)intact=false;}producer.join();check(intact,"concurrent publication is coherent and ordered");}
    {field::RtKeyboard keyboard;field::KeyboardEvent event;watching=true;bool a=keyboard.noteOn(2,60,.5f),b=keyboard.isNoteOn(2,60),c=keyboard.pop(event);watching=false;check(a&&b&&c&&event.status==0x91&&event.data1==60&&event.data2==64,"GUI handoff");
    keyboard.observe(0x92,60,127);keyboard.noteOff(2,60,0);check(!keyboard.isNoteOn(2,60)&&keyboard.isNoteOn(3,60),"GUI release preserves incoming channel state");keyboard.observe(0x82,60,0);check(!keyboard.isNoteOnForChannels(0xffff,60),"incoming release");keyboard.pop(event);
    check(!keyboard.noteOn(0,60,1)&&!keyboard.noteOn(1,128,1)&&!keyboard.noteOn(1,60,std::numeric_limits<float>::infinity()),"invalid events rejected");check(keyboard.noteOn(1,60,std::numeric_limits<float>::max())&&keyboard.pop(event)&&event.data2==127,"large finite velocity clamps before conversion");
    keyboard.noteOn(1,60,1);keyboard.allNotesOff(0);keyboard.noteOn(1,64,1);check(keyboard.pop(event)&&event.status==0&&keyboard.pop(event)&&event.data1==64&&keyboard.isGuiNoteOn(1,64)&&!keyboard.pop(event),"panic fence preserves post-request notes and coherent GUI keys");
    {field::RtQueue<int,8> late;int value=0;late.push(1);const auto fence=late.producerPosition();late.push(2);late.pop(value);late.pop(value);late.push(3);late.discardThrough(fence);check(late.pop(value)&&value==3&&!late.pop(value),"late panic fence cannot rewind already consumed queue data");}
    for(int i=0;i<512;++i)check(keyboard.noteOn(1,i%128,.8f),"fill keyboard queue");check(!keyboard.noteOn(1,60,1)&&keyboard.overflowCount()==1,"keyboard backpressure requests panic");watching=true;const bool panic=keyboard.pop(event)&&event.status==0,empty=!keyboard.pop(event);watching=false;check(panic&&empty&&!keyboard.isNoteOnForChannels(0xffff,60),"reserved panic discards stale notes and clears keys");
    check(keyboard.noteOn(4,65,1)&&keyboard.pop(event)&&event.status==0x93,"recovery after panic");keyboard.allNotesOff(0);check(keyboard.pop(event)&&event.status==0,"explicit panic cannot be lost to queue saturation");}
    {field::RealtimeValues<2> values;values.initialize(0,.25f);values.initialize(1,0);field::RealtimeValues<2>::Snapshot first,second;
    check(!values.pending(0,first),"initial parameter mirror acknowledged");watching=true;values.fromAudio(0,.5f);values.pending(0,first);values.fromAudio(0,.75f);values.acknowledge(0,first);const bool newer=values.pending(0,second);watching=false;check(newer&&second.value==.75f&&values.load(0)==.75f,"stale GUI acknowledgement cannot overwrite newer audio value");
    values.fromControl(0,.3f);check(!values.pending(0,first)&&values.load(0)==.3f,"host change replaces pending MIDI update");values.acknowledge(0,second);check(values.pending(0,first)&&first.value==.3f,"late acknowledgement only schedules current value again");
    const auto controlVersion=values.version(0);const auto controlRead=values.load(0);values.fromAudio(0,.9f);check(!values.fromControlIfUnchanged(0,controlVersion,controlRead)&&values.pending(0,second)&&second.value==.9f,"control acknowledgement cannot clear MIDI arriving after its native read");
    const auto currentVersion=values.version(0);check(values.fromControlIfUnchanged(0,currentVersion,.2f)&&!values.pending(0,second),"uncontended control notification can acknowledge its own generation");values.fromAudio(0,.8f);check(values.pending(0,second)&&second.value==.8f,"MIDI arriving after a completed control acknowledgement stays pending");
    const auto abaVersion=values.version(0);const auto abaValue=values.load(0);values.fromAudio(0,.6f);values.fromAudio(0,abaValue);check(!values.fromControlIfUnchanged(0,abaVersion,abaValue)&&values.pending(0,second),"full generation token detects A to B to A notifications");
    std::atomic<bool> done{false};std::thread audio([&]{for(int i=1;i<=100000;++i)values.fromAudio(1,(float)i);done=true;});while(!done.load()){field::RealtimeValues<2>::Snapshot snapshot;if(values.pending(1,snapshot))values.acknowledge(1,snapshot);}audio.join();check(values.load(1)==100000,"concurrent mirror never changes audio authority");}
    check(allocations==0&&frees==0,"audio-facing handoff neither allocates nor frees");
    std::cout<<"PASS: bounded FIFO, backpressure/wrap, concurrent publication, keyboard state, panic recovery, allocation/free guard\n";
}
