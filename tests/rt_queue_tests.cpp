#include "RtKeyboard.h"
#include "ControlJobs.h"
#include "EffectAnimation.h"
#include <chrono>
#include "RealtimeValues.h"
#include "RealtimeSnapshot.h"
#include "MidiLearn.h"
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
    {struct State {uint64_t sequence=0;std::array<uint64_t,64> values{};};field::RealtimeSnapshot<State> snapshots;std::atomic<bool> finished{false};std::thread producer([&]{for(uint64_t n=1;n<=100000;++n){State state;state.sequence=n;for(size_t i=0;i<state.values.size();++i)state.values[i]=n+i;snapshots.publish(state);}finished.store(true,std::memory_order_release);});while(!finished.load(std::memory_order_acquire)){const auto state=snapshots.read();if(state.sequence)for(size_t i=0;i<state.values.size();++i)check(state.values[i]==state.sequence+i,"UI snapshot was torn across producer/consumer slots");}producer.join();check(snapshots.read().sequence==100000,"UI snapshot did not deliver latest state");}

    {field::CwoAnimation slow,fast,longDelay;slow.advance(0,0,.2);fast.advance(1,0,.2);longDelay.advance(0,1,.2);check(fast.frequencyTurns>slow.frequencyTurns,"CWO frequency changes wheel rate");check(longDelay.flowTurns<slow.flowTurns,"CWO delay changes flow rate");auto held=fast;fast.advance(1,1,0);check(fast.frequencyTurns==held.frequencyTurns&&fast.flowTurns==held.flowTurns,"zero elapsed time cannot move animation");for(int i=0;i<10000;++i)fast.advance(1,0,.1);check(fast.frequencyTurns>=0&&fast.frequencyTurns<1&&fast.flowTurns>=0&&fast.flowTurns<1,"CWO phases wrap safely");field::CwoAnimation chunks,one;for(int i=0;i<10;++i)chunks.advance(.6f,.3f,.02);one.advance(.6f,.3f,.2);check(std::abs(chunks.frequencyTurns-one.frequencyTurns)<1e-6&&std::abs(chunks.flowTurns-one.flowTurns)<1e-6,"animation rates do not depend on timer chunk size");const auto small=field::delayCircle(125),middle=field::delayCircle(250),large=field::delayCircle(1000);check(small.radius<middle.radius&&middle.radius<large.radius&&small.x>middle.x&&middle.x>large.x,"Delay Range changes selected ring and red centre together");}

    {field::ControlJobs jobs;std::atomic<bool> entered{false},release{false},published{false};
    auto wait=[&](auto predicate){auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(!predicate()){check(std::chrono::steady_clock::now()<deadline,"background job timeout");std::this_thread::yield();}};
    check(jobs.submit("save",[&]{entered=true;while(!release.load())std::this_thread::yield();if(jobs.beginCommit())published=true;}),"admit file job");
    wait([&]{return entered.load();});check(!jobs.submit("second",[]{}),"bounded worker rejects backlog");check(jobs.cancel(),"cancel preparation");release=true;wait([&]{return !jobs.busy();});check(!published&&jobs.status()=="save cancelled","cancel prevents destination publication");
    entered=false;release=false;
    check(jobs.submit("export",[&]{check(jobs.beginCommit(),"begin file publication");entered=true;while(!release.load())std::this_thread::yield();published=true;}),"worker reusable");
    wait([&]{return entered.load();});check(!jobs.cancel(),"publication cannot be cancelled halfway");release=true;wait([&]{return !jobs.busy();});check(published&&jobs.status()=="export finished","publication completes exactly once");
    jobs.submit("failure",[]{throw std::runtime_error("disk full");});wait([&]{return !jobs.busy();});check(jobs.status()=="failure: disk full","worker reports exceptions");
    jobs.shutdown();check(!jobs.submit("closed",[]{}),"shutdown rejects new work");}

    {field::ControlJobs jobs;std::atomic<bool> staged{false},published{false};const auto control=std::this_thread::get_id();
     auto wait=[&](auto predicate){const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(!predicate()){check(std::chrono::steady_clock::now()<deadline,"staged job timeout");std::this_thread::yield();}};
     check(jobs.submit("load",[&]{staged=true;jobs.applyOnControl([&]{check(std::this_thread::get_id()==control,"commit ran on worker");published=true;});}),"admit staged job");
     wait([&]{return staged.load();});check(jobs.cancel(),"cancel staged preparation");wait([&]{jobs.serviceCommit();return !jobs.busy();});check(!published&&jobs.status()=="load cancelled","cancelled staged job published");
     staged=false;check(jobs.submit("edit",[&]{staged=true;check(jobs.applyOnControl([&]{check(!jobs.cancel(),"cancel admitted halfway through commit");published=true;}),"commit rejected");}),"reuse staged worker");
     wait([&]{return staged.load();});wait([&]{jobs.serviceCommit();return !jobs.busy();});check(published&&jobs.status()=="edit finished","staged publication did not finish");
     check(jobs.submit("failure",[&]{jobs.applyOnControl([]{throw std::runtime_error("commit failed");});}),"admit failing staged job");wait([&]{jobs.serviceCommit();return !jobs.busy();});check(jobs.status()=="failure: commit failed","commit exception not returned to worker");
     staged=false;check(jobs.submit("shutdown",[&]{staged=true;jobs.applyOnControl([]{});}),"admit shutdown job");wait([&]{return staged.load();});jobs.shutdown();check(!jobs.busy(),"shutdown did not reject pending commit");}

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
    {field::MidiLearn<4> maps;std::array<float,4> values{.5f,.5f,.5f,.5f};auto event=[&](int channel,int cc,int value){watching=true;const bool used=maps.process(channel,cc,value,[&](size_t target){return values[target];},[&](size_t target,float next){values[target]=next;});watching=false;return used;};
    check(maps.arm(0,field::EncoderMode::absolute,true)&&maps.learningTarget()==0,"arm CC learn");check(!event(1,64,127)&&maps.learningTarget()==0,"sustain cannot accidentally consume a learn assignment");check(event(3,21,0)&&maps.learningTarget()<0&&values[0]==.5f,"learn captures source/channel without moving sound");check(maps.binding(0).channel==3&&maps.binding(0).cc==21,"learned address persists");check(!event(2,21,127)&&values[0]==.5f,"another channel does not match");check(event(3,21,0)&&values[0]==.5f,"pickup prevents initial jump");event(3,21,90);check(std::abs(values[0]-90/127.f)<1e-6,"pickup accepts crossing target");values[0]=.2f;event(3,21,100);check(values[0]==.2f,"external automation re-arms pickup");event(3,21,10);check(std::abs(values[0]-10/127.f)<1e-6,"pickup crosses new automated value");
    for(unsigned mode=1;mode<4;++mode){maps.assign(1,{1,22,(field::EncoderMode)mode,false,false});values[1]=.5f;event(1,22,mode==2?65:1);check(values[1]>.5f,"relative turn increments");event(1,22,mode==1?127:mode==2?63:65);check(std::abs(values[1]-.5f)<1e-6,"relative opposite turn decrements");}
    maps.assign(2,{0,23,field::EncoderMode::twosComplement,false,true});values[2]=.5f;event(16,23,127);check(values[2]>.5f,"reversed relative direction and omni mapping");maps.assign(2,{0,23,field::EncoderMode::absolute,false,true});event(1,23,0);check(values[2]==1,"absolute direction inversion");
    maps.arm(3,field::EncoderMode::absolute,false);maps.cancel();event(1,24,127);check(maps.binding(3).cc<0,"cancel prevents stale learn");maps.arm(3,field::EncoderMode::absolute,false);maps.clear(3);event(1,24,127);check(maps.binding(3).cc<0,"intervening clear cannot be resurrected by pending learn");check(!maps.assign(0,{17,2})&&!maps.assign(0,{1,120})&&!maps.assign(0,{1,64}),"invalid mapping addresses rejected");
    maps.assign(3,{1,25,field::EncoderMode::twosComplement,false});values[3]=0;for(int i=0;i<64;++i)event(1,25,1);check(values[3]>.5f,"relative steps accumulate rather than resetting");maps.clearAll();check(maps.binding(0).cc<0&&!event(3,21,100),"clearing removes source interception");}
    {field::MidiLearn<4> maps;std::array<float,4> values{.5f,.5f,.5f,.5f};std::atomic<bool> finished{false};std::thread controls([&]{for(int i=0;i<50000;++i){maps.assign(i%4,{i%16+1,i%64,(field::EncoderMode)(i%4),true,(i%2)!=0});if(i%3==0)maps.arm((i+1)%4,field::EncoderMode::absolute,true);if(i%7==0)maps.clear((i+2)%4);}maps.cancel();finished.store(true,std::memory_order_release);});int tick=0;while(!finished.load(std::memory_order_acquire)){watching=true;maps.process(tick%16+1,tick%64,tick%128,[&](size_t target){return values[target];},[&](size_t target,float next){values[target]=next;});watching=false;++tick;}controls.join();for(float value:values)check(std::isfinite(value)&&value>=0&&value<=1,"concurrent mapping edits preserve bounded values");maps.assign(0,{2,26,field::EncoderMode::absolute,false});maps.process(2,26,127,[&](size_t target){return values[target];},[&](size_t target,float next){values[target]=next;});check(values[0]==1,"final control assignment remains visible after concurrent learn/edit traffic");}
    check(allocations==0&&frees==0,"MIDI learn/pickup/relative processing neither allocates nor frees");
    std::cout<<"PASS: bounded FIFO, backpressure/wrap, concurrent publication, keyboard state, panic recovery, allocation/free guard\n";
}
