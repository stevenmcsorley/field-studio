#include "TapeStorage.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>
#include <utility>
#include <vector>
#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace {thread_local bool watch=false;std::atomic<size_t> allocations{0},frees{0};}
void* operator new(std::size_t n){if(watch)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void* operator new(std::size_t n,std::align_val_t alignment){
    if(watch)++allocations;
    const auto a=static_cast<size_t>(alignment);
#ifdef _MSC_VER
    if(auto* p=_aligned_malloc(n?n:1,a))return p;
#else
    void* p=nullptr;if(posix_memalign(&p,a,n?n:1)==0)return p;
#endif
    throw std::bad_alloc();
}
void* operator new[](std::size_t n,std::align_val_t alignment){return ::operator new(n,alignment);}
void operator delete(void* p)noexcept{if(watch&&p)++frees;std::free(p);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,std::size_t)noexcept{::operator delete(p);}
void operator delete[](void* p,std::size_t)noexcept{::operator delete(p);}
void operator delete(void* p,std::align_val_t)noexcept{
    if(watch&&p)++frees;
#ifdef _MSC_VER
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p,std::align_val_t a)noexcept{::operator delete(p,a);}
void operator delete(void* p,std::size_t,std::align_val_t a)noexcept{::operator delete(p,a);}
void operator delete[](void* p,std::size_t,std::align_val_t a)noexcept{::operator delete(p,a);}
using namespace field::tape;
namespace {
void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
template<class F>void rt(F fn){const auto a=allocations.load(),d=frees.load();watch=true;fn();watch=false;check(a==allocations.load()&&d==frees.load(),"realtime operation performs neither heap allocation nor free");}
void clean(TapePool& pool){pool.collect();const auto s=pool.stats();check(s.freePages==s.pageCapacity&&s.freeNodes==s.nodeCapacity&&s.pagesInUse==0&&s.nodesInUse==0,"worker collection returns all unreachable storage");}
uint32_t randomState=0x12345678;uint32_t nextRandom(){randomState^=randomState<<13;randomState^=randomState>>17;randomState^=randomState<<5;return randomState;}
struct Envelope {TapeBuffer tape;std::array<float,64> expected{};};
struct SnapshotQueue {
    std::array<Envelope,16> slots;
    std::atomic<size_t> read{0},write{0};
    bool push(Envelope&& value){auto w=write.load(std::memory_order_relaxed),next=(w+1)%slots.size();if(next==read.load(std::memory_order_acquire))return false;slots[w]=std::move(value);write.store(next,std::memory_order_release);return true;}
    bool pop(Envelope& value){auto r=read.load(std::memory_order_relaxed);if(r==write.load(std::memory_order_acquire))return false;value=std::move(slots[r]);read.store((r+1)%slots.size(),std::memory_order_release);return true;}
};
}
int main(){
    {
        TapePool pool(8,32);TapeBuffer tape(pool);rt([&]{check(tape.resizeLogical(maxTapeFrames),"logical growth reaches six minutes at192k");check(tape.rootIdentity()==0&&pool.stats().pagesInUse==0,"logical growth allocates no pages");check(tape.read(0,maxTapeFrames-1)==0,"sparse tape reads zero");check(tape.writeStereo(maxTapeFrames-1,.25f,-.5f),"far write succeeds");check(tape.read(0,maxTapeFrames-1)==.25f&&tape.read(1,maxTapeFrames-1)==-.5f,"far sparse write retains stereo data");check(tape.read(0,maxTapeFrames-2)==0&&tape.read(0,0)==0,"far seek never fills old gaps with data");check(pool.stats().pagesInUse==1&&pool.stats().nodesInUse==3,"far write touches one page and fixed-depth path");auto root=tape.rootIdentity();check(!tape.writeStereo(maxTapeFrames,1,1)&&tape.rootIdentity()==root,"frame limit rejects writes without mutation");tape.clear();});clean(pool);
    }
    {
        TapePool pool(32,128);TapeBuffer tape(pool);check(tape.writeStereo(17,.1f,-.2f)&&tape.writeStereo(pageFrames+2,.3f,-.4f),"initial two pages");TapeBuffer snapshot;rt([&]{const auto before=pool.stats();snapshot=tape.snapshot();check(snapshot.rootIdentity()==tape.rootIdentity(),"snapshot retains root identity");check(pool.stats().pagesInUse==before.pagesInUse&&pool.stats().nodesInUse==before.nodesInUse,"snapshot copies no page or node");check(tape.writeStereo(17,.8f,-.9f),"shared page COW write succeeds");check(snapshot.read(0,17)==.1f&&snapshot.read(1,17)==-.2f,"snapshot samples remain unchanged");check(tape.read(0,17)==.8f&&tape.read(1,pageFrames+2)==-.4f,"COW updates only requested stereo frame");check(pool.stats().pagesInUse==3&&pool.stats().nodesInUse==6,"COW copies exactly one page and three nodes");auto copied=snapshot.snapshot();copied.clear();tape.clear();snapshot.clear();});check(pool.stats().retiredNodes>0,"final releases defer reclamation");check(pool.collect(1)==1,"worker reclamation obeys object budget");clean(pool);
    }
    {
        TapePool pool(1,16);TapeBuffer tape(pool);check(tape.writeStereo(0,.2f,.3f),"one-page pool initial write");auto snapshot=tape.snapshot();const auto root=tape.rootIdentity(),size=tape.size();rt([&]{check(!tape.writeStereo(0,.4f,.5f),"page exhaustion rejects shared write");check(tape.rootIdentity()==root&&tape.size()==size&&tape.read(0,0)==.2f,"failed COW is transactional");snapshot.clear();check(tape.writeStereo(0,.4f,.5f),"unique page needs no spare capacity");tape.clear();});clean(pool);
    }
    {
        TapePool pool(8,3);TapeBuffer tape(pool);check(tape.writeStereo(0,.2f,.3f),"three-node pool initial write");auto snapshot=tape.snapshot();rt([&]{check(!tape.writeStereo(0,.4f,.5f),"node exhaustion rejects shared write");check(tape.read(0,0)==.2f,"node exhaustion preserves old audio");snapshot.clear();check(tape.writeStereo(0,.4f,.5f),"unique path reuses its nodes");tape.clear();});clean(pool);
    }
    {
        TapePool pool(32,128);TapeBuffer tape(pool);const uint64_t points[]{0,4095,4096,4999,5000,8191,8192,262144,16777216,maxTapeFrames-1};for(size_t i=0;i<10;++i)check(tape.writeStereo(points[i],static_cast<float>(i+1),-static_cast<float>(i+1)),"boundary write succeeds");auto old=tape.snapshot();rt([&]{check(tape.resizeLogical(5000),"shared tail truncation succeeds");check(tape.read(0,4999)==4&&tape.read(0,5000)==0,"truncation retains prefix");check(tape.resizeLogical(maxTapeFrames),"regrowth succeeds");for(auto p:points)if(p>=5000)check(tape.read(0,p)==0,"regrowth cannot resurrect discarded pages");check(old.read(0,maxTapeFrames-1)==10&&old.read(0,5000)==5,"truncation cannot affect old generation");check(tape.writeStereo(maxTapeFrames-1,.75f,.5f),"write after sparse regrowth succeeds");check(tape.resizeLogical(4096)&&tape.resizeLogical(maxTapeFrames),"exact-page-boundary truncation succeeds");check(tape.read(0,4095)==2&&tape.read(0,4096)==0,"exact-page boundary keeps correct page");tape.clear();old.clear();});clean(pool);
    }
    {
        TapePool pool(16,64);TapeBuffer tape(pool);check(tape.writeStereo(0,1,1)&&tape.writeStereo(maxTapeFrames-1,2,2),"sparse endpoints");auto old=tape.snapshot();rt([&]{check(tape.resizeLogical(500000)&&tape.resizeLogical(maxTapeFrames),"truncate through missing subtree succeeds");check(tape.read(0,0)==1&&tape.read(0,maxTapeFrames-1)==0&&old.read(0,maxTapeFrames-1)==2,"missing-subtree truncate prunes later branches");tape.clear();old.clear();});clean(pool);
    }
    {
        TapePool pool(128,1024);TapeBuffer tape(pool,20000);std::vector<float> left,right;
        struct Saved {TapeBuffer tape;std::vector<float> l,r;};std::vector<Saved> history;
        for(int iteration=0;iteration<1800;++iteration){const auto op=nextRandom()%5;
            if(op<3){const uint64_t frame=nextRandom()%20000;const float v=static_cast<float>(nextRandom()%1000)/1000;rt([&]{check(tape.writeStereo(frame,v,-v),"model write succeeds");});if(left.size()<=frame){left.resize(frame+1,0);right.resize(frame+1,0);}left[frame]=v;right[frame]=-v;}
            else if(op==3){const uint64_t length=nextRandom()%20000;rt([&]{check(tape.resizeLogical(length),"model resize succeeds");});left.resize(length,0);right.resize(length,0);}
            else{if(history.size()==12)history.erase(history.begin());history.push_back({tape.snapshot(),left,right});}
            if(iteration%25==0){check(tape.size()==left.size(),"model logical size agrees");for(size_t i=0;i<left.size();i+=31)check(tape.read(0,i)==left[i]&&tape.read(1,i)==right[i],"random edits match vector oracle");for(const auto& saved:history)for(size_t i=0;i<saved.l.size();i+=53)check(saved.tape.read(0,i)==saved.l[i]&&saved.tape.read(1,i)==saved.r[i],"historical snapshots match immutable oracles");}
            pool.collect();
        }
        history.clear();tape.clear();clean(pool);
    }
    {
        TapePool pool(512,4096);TapeBuffer writer(pool);for(int i=0;i<64;++i)check(writer.writeStereo(static_cast<uint64_t>(i)*pageFrames,0,0),"threaded initial page");SnapshotQueue queue;std::atomic<bool> done{false},failed{false};std::atomic<size_t> inspected{0};
        std::thread worker([&]{Envelope item;while(!done.load(std::memory_order_acquire)){if(queue.pop(item)){for(int pass=0;pass<3;++pass){for(int page=0;page<64;++page){const auto v=item.tape.readStereo(static_cast<uint64_t>(page)*pageFrames);if(v[0]!=item.expected[static_cast<size_t>(page)]||v[1]!=-item.expected[static_cast<size_t>(page)])failed=true;}std::this_thread::yield();}item.tape.clear();++inspected;}pool.collect(64);std::this_thread::yield();}while(queue.pop(item)){for(int page=0;page<64;++page)if(item.tape.read(0,static_cast<uint64_t>(page)*pageFrames)!=item.expected[static_cast<size_t>(page)])failed=true;item.tape.clear();++inspected;pool.collect(64);}pool.collect();});
        std::array<float,64> expected{};const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(30);
        for(int iteration=0;iteration<20000;++iteration){const auto page=static_cast<size_t>((iteration*17)%64);const float value=static_cast<float>(iteration+1);bool written=false;do{rt([&]{written=writer.writeStereo(page*pageFrames,value,-value);});if(!written){check(std::chrono::steady_clock::now()<deadline,"reclaimer makes progress under pressure");std::this_thread::yield();}}while(!written);expected[page]=value;rt([&]{Envelope item;item.tape=writer.snapshot();item.expected=expected;queue.push(std::move(item));});}
        done.store(true,std::memory_order_release);worker.join();check(!failed.load()&&inspected.load()>0,"concurrent snapshot readers/reclaimer observe immutable complete generations");writer.clear();clean(pool);
    }
    std::cout<<"PASS: sparse bounds, constant-time snapshots, fixed-depth COW, transactional exhaustion, zero-tail truncation, vector oracle, deferred reclamation, allocation/free-free RT operations and threaded snapshot stress\n";
}
