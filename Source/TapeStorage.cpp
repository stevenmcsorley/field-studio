#include "TapeStorage.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <mutex>

namespace field::tape {
namespace {
std::atomic<size_t> reservedTapeBytes{0},tapeByteLimit{size_t{2}*1024*1024*1024};
std::mutex tapeBudgetMutex;
using Token=uint64_t;
constexpr uint32_t fanout=64;
static_assert(std::atomic<uint64_t>::is_always_lock_free,"Tape storage requires lock-free64bit atomics");
static_assert(std::atomic<uint32_t>::is_always_lock_free,"Tape storage requires lock-free32bit atomics");
Token token(uint32_t index,uint32_t generation) noexcept{return static_cast<Token>(generation)<<32|(static_cast<Token>(index)+1);}
uint32_t indexOf(Token h) noexcept{return static_cast<uint32_t>(h)-1;}
uint32_t generationOf(Token h) noexcept{return static_cast<uint32_t>(h>>32);}

// A worker produces recycled entries; the single writer consumes them. No
// compare/exchange loop or shared freelist means no ABA problem on reuse.
class FreeIds {
public:
    explicit FreeIds(size_t capacity):slots(capacity+1),ids(new uint32_t[slots]{}){
        for(size_t i=0;i<capacity;++i){const bool ok=push(static_cast<uint32_t>(i));(void)ok;assert(ok);}
    }
    bool pop(uint32_t& id) noexcept{
        const size_t r=read.load(std::memory_order_relaxed);
        if(r==write.load(std::memory_order_acquire))return false;
        id=ids[r];read.store((r+1)%slots,std::memory_order_release);return true;
    }
    bool push(uint32_t id) noexcept{
        const size_t w=write.load(std::memory_order_relaxed),next=(w+1)%slots;
        if(next==read.load(std::memory_order_acquire))return false;
        ids[w]=id;write.store(next,std::memory_order_release);return true;
    }
    size_t available() const noexcept{
        const size_t r=read.load(std::memory_order_acquire),w=write.load(std::memory_order_acquire);
        return w>=r?w-r:slots-r+w;
    }
private:
    size_t slots;
    std::unique_ptr<uint32_t[]> ids;
    alignas(64) std::atomic<size_t> read{0};
    alignas(64) std::atomic<size_t> write{0};
};
struct alignas(64) Page {
    std::atomic<uint32_t> refs{0},generation{1};
    std::array<float,pageFrames*2> samples{};
};
struct alignas(64) Node {
    std::atomic<uint32_t> refs{0},generation{1};
    uint32_t level=0; // 2=root,1=middle,0=leaf holding sample pages.
    std::array<Token,fanout> children{};
};
size_t checkedCapacity(size_t value){if(value>1000000)throw std::invalid_argument("Tape pool capacity exceeds one million objects");return value;}
unsigned firstBit(uint64_t bits) noexcept{unsigned result=0;while((bits&1)==0){bits>>=1;++result;}return result;}
}

struct TapePool::Reservation {
    size_t bytes;
    explicit Reservation(size_t count):bytes(count){
        std::lock_guard<std::mutex> lock(tapeBudgetMutex);
        const auto used=reservedTapeBytes.load(std::memory_order_relaxed),limit=tapeByteLimit.load(std::memory_order_relaxed);
        if(count>limit||used>limit-count)throw std::runtime_error("Tape memory budget exceeded. Clear unused tapes/undo history, or increase COM > Tape memory budget.");
        reservedTapeBytes.fetch_add(count,std::memory_order_relaxed);
    }
    ~Reservation(){reservedTapeBytes.fetch_sub(bytes,std::memory_order_relaxed);}
};
size_t TapePool::memoryUsed() noexcept {return reservedTapeBytes.load(std::memory_order_relaxed);}
size_t TapePool::memoryLimit() noexcept {return tapeByteLimit.load(std::memory_order_relaxed);}
bool TapePool::setMemoryLimit(size_t bytes){std::lock_guard<std::mutex> lock(tapeBudgetMutex);if(bytes<reservedTapeBytes.load(std::memory_order_relaxed))return false;tapeByteLimit.store(bytes,std::memory_order_relaxed);return true;}
size_t TapePool::estimateBytes(size_t pages,size_t nodes){
    checkedCapacity(pages);checkedCapacity(nodes);
    const uint64_t bytes=uint64_t(pages)*sizeof(Page)+uint64_t(nodes)*sizeof(Node)+((uint64_t(pages)+63)/64+(uint64_t(nodes)+63)/64)*sizeof(std::atomic<uint64_t>)+(uint64_t(pages)+nodes+2)*sizeof(uint32_t)+1024;
    if(bytes>std::numeric_limits<size_t>::max())throw std::length_error("Tape pool size overflow");return static_cast<size_t>(bytes);
}

struct TapePool::Impl {
    std::mutex collectorMutex;
    const size_t pageCount,nodeCount,pageWords,nodeWords;
    std::unique_ptr<Page[]> pages;
    std::unique_ptr<Node[]> nodes;
    std::unique_ptr<std::atomic<uint64_t>[]> retiredPages,retiredNodes;
    FreeIds freePages,freeNodes;
    std::atomic<size_t> pageUse{0},nodeUse{0},pageRetired{0},nodeRetired{0};
    Impl(size_t pc,size_t nc):pageCount(checkedCapacity(pc)),nodeCount(checkedCapacity(nc)),pageWords((pc+63)/64),nodeWords((nc+63)/64),
        pages(new Page[pc]),nodes(new Node[nc]),retiredPages(new std::atomic<uint64_t>[pageWords]),retiredNodes(new std::atomic<uint64_t>[nodeWords]),freePages(pc),freeNodes(nc){
        // Volatile stores force backing pages to be touched even if allocation
        // or zero-fill is optimized to lazy calloc/memset by the runtime.
        for(size_t i=0;i<pageCount;++i){pages[i].samples.fill(0);volatile float* data=pages[i].samples.data();for(size_t j=0;j<pages[i].samples.size();j+=1024)data[j]=0;data[pages[i].samples.size()-1]=0;}
        for(size_t i=0;i<nodeCount;++i){nodes[i].children.fill(0);volatile Token* data=nodes[i].children.data();data[0]=0;data[fanout-1]=0;}
        for(size_t i=0;i<pageWords;++i)retiredPages[i].store(0,std::memory_order_relaxed);
        for(size_t i=0;i<nodeWords;++i)retiredNodes[i].store(0,std::memory_order_relaxed);
    }
    Node& node(Token h) noexcept{assert(h&&indexOf(h)<nodeCount);auto& n=nodes[indexOf(h)];assert(n.generation.load(std::memory_order_relaxed)==generationOf(h));return n;}
    const Node& node(Token h)const noexcept{assert(h&&indexOf(h)<nodeCount);const auto& n=nodes[indexOf(h)];assert(n.generation.load(std::memory_order_relaxed)==generationOf(h));return n;}
    Page& page(Token h) noexcept{assert(h&&indexOf(h)<pageCount);auto& p=pages[indexOf(h)];assert(p.generation.load(std::memory_order_relaxed)==generationOf(h));return p;}
    const Page& page(Token h)const noexcept{assert(h&&indexOf(h)<pageCount);const auto& p=pages[indexOf(h)];assert(p.generation.load(std::memory_order_relaxed)==generationOf(h));return p;}
    void retainNode(Token h) noexcept{if(h){const auto old=node(h).refs.fetch_add(1,std::memory_order_relaxed);assert(old>0);(void)old;}}
    void retainPage(Token h) noexcept{if(h){const auto old=page(h).refs.fetch_add(1,std::memory_order_relaxed);assert(old>0);(void)old;}}
    void releaseNode(Token h) noexcept{
        if(!h)return;
        const auto old=node(h).refs.fetch_sub(1,std::memory_order_acq_rel);assert(old>0);
        if(old==1){nodeRetired.fetch_add(1,std::memory_order_relaxed);retiredNodes[indexOf(h)/64].fetch_or(uint64_t{1}<<(indexOf(h)%64),std::memory_order_release);}
    }
    void releasePage(Token h) noexcept{
        if(!h)return;
        const auto old=page(h).refs.fetch_sub(1,std::memory_order_acq_rel);assert(old>0);
        if(old==1){pageRetired.fetch_add(1,std::memory_order_relaxed);retiredPages[indexOf(h)/64].fetch_or(uint64_t{1}<<(indexOf(h)%64),std::memory_order_release);}
    }
    Token allocateNode(uint32_t level) noexcept{
        uint32_t id=0;if(!freeNodes.pop(id))return 0;auto& n=nodes[id];assert(n.refs.load(std::memory_order_relaxed)==0);n.level=level;
        n.refs.store(1,std::memory_order_relaxed);nodeUse.fetch_add(1,std::memory_order_relaxed);return token(id,n.generation.load(std::memory_order_relaxed));
    }
    Token allocatePage() noexcept{
        uint32_t id=0;if(!freePages.pop(id))return 0;auto& p=pages[id];assert(p.refs.load(std::memory_order_relaxed)==0);
        p.refs.store(1,std::memory_order_relaxed);pageUse.fetch_add(1,std::memory_order_relaxed);return token(id,p.generation.load(std::memory_order_relaxed));
    }
    struct Plan {size_t nodes=0,pages=0;};
    Plan plan(Token root,uint64_t pageIndex,bool createMissing,bool modifyPage)const noexcept{
        Plan result;Token h=root;bool ancestorCopy=false;
        for(int level=2;level>=0;--level){
            if(!h&&!createMissing)return result;
            const bool copy=!h||ancestorCopy||node(h).refs.load(std::memory_order_acquire)>1;
            if(copy)++result.nodes;
            ancestorCopy=copy;
            const auto slot=static_cast<size_t>((pageIndex>>(level*6))&63);
            h=h?node(h).children[slot]:0;
        }
        if(modifyPage && (h||createMissing) && (!h||ancestorCopy||page(h).refs.load(std::memory_order_acquire)>1))++result.pages;
        return result;
    }
    bool available(const Plan& p)const noexcept{return freeNodes.available()>=p.nodes&&freePages.available()>=p.pages;}
    Token uniqueNode(Token old,uint32_t level) noexcept{
        if(old&&node(old).refs.load(std::memory_order_acquire)==1)return old;
        const Token fresh=allocateNode(level);assert(fresh);auto& dest=node(fresh);
        if(old){dest.children=node(old).children;for(Token child:dest.children)if(level)retainNode(child);else retainPage(child);}
        return fresh;
    }
    Token uniquePage(Token old) noexcept{
        if(old&&page(old).refs.load(std::memory_order_acquire)==1)return old;
        const Token fresh=allocatePage();assert(fresh);
        if(old)std::memcpy(page(fresh).samples.data(),page(old).samples.data(),sizeof(float)*pageFrames*2);
        return fresh;
    }
    void replaceNode(Token& link,Token value)noexcept{if(link!=value){const Token old=link;link=value;releaseNode(old);}}
    void replacePage(Token& link,Token value)noexcept{if(link!=value){const Token old=link;link=value;releasePage(old);}}
    Token findPage(Token root,uint64_t pageIndex)const noexcept{
        Token h=root;
        for(int level=2;level>=0;--level){if(!h)return 0;h=node(h).children[static_cast<size_t>((pageIndex>>(level*6))&63)];}
        return h;
    }
    bool write(Token& root,uint64_t frame,float left,float right) noexcept{
        const uint64_t pageIndex=frame/pageFrames;const auto needed=plan(root,pageIndex,true,true);
        // Single consumer means resources cannot disappear after this preflight;
        // the concurrent collector can only append more free IDs.
        if(!available(needed))return false;
        replaceNode(root,uniqueNode(root,2));Node* n=&node(root);
        for(int level=2;level>0;--level){auto& link=n->children[static_cast<size_t>((pageIndex>>(level*6))&63)];replaceNode(link,uniqueNode(link,static_cast<uint32_t>(level-1)));n=&node(link);}
        auto& link=n->children[static_cast<size_t>(pageIndex&63)];replacePage(link,uniquePage(link));auto& samples=page(link).samples;
        const auto at=static_cast<size_t>(frame%pageFrames);samples[at]=std::isfinite(left)?left:0;samples[pageFrames+at]=std::isfinite(right)?right:0;return true;
    }
    bool truncate(Token& root,uint64_t frames) noexcept{
        if(!root)return true;
        const uint64_t pageIndex=(frames-1)/pageFrames;const size_t offset=static_cast<size_t>(frames%pageFrames);
        const auto needed=plan(root,pageIndex,false,offset!=0);if(!available(needed))return false;
        Token* current=&root;
        for(int level=2;level>=0;--level){
            if(!*current)break;
            replaceNode(*current,uniqueNode(*current,static_cast<uint32_t>(level)));auto& n=node(*current);
            const auto slot=static_cast<size_t>((pageIndex>>(level*6))&63);
            for(size_t i=slot+1;i<fanout;++i){const Token old=n.children[i];n.children[i]=0;if(level)releaseNode(old);else releasePage(old);}
            if(level){current=&n.children[slot];continue;}
            if(offset && n.children[slot]){auto& link=n.children[slot];replacePage(link,uniquePage(link));auto& samples=page(link).samples;std::fill(samples.begin()+static_cast<std::ptrdiff_t>(offset),samples.begin()+pageFrames,0);std::fill(samples.begin()+pageFrames+static_cast<std::ptrdiff_t>(offset),samples.end(),0);}
        }
        return true;
    }
    void reclaimNode(uint32_t id) noexcept{
        auto& n=nodes[id];assert(n.refs.load(std::memory_order_acquire)==0);
        for(Token h:n.children)if(n.level)releaseNode(h);else releasePage(h);
        n.children.fill(0);n.level=0;
        uint32_t next=n.generation.load(std::memory_order_relaxed)+1;if(next==0)next=1;n.generation.store(next,std::memory_order_relaxed);
        nodeRetired.fetch_sub(1,std::memory_order_relaxed);nodeUse.fetch_sub(1,std::memory_order_relaxed);
        const bool ok=freeNodes.push(id);assert(ok);(void)ok;
    }
    void reclaimPage(uint32_t id) noexcept{
        auto& p=pages[id];assert(p.refs.load(std::memory_order_acquire)==0);p.samples.fill(0);
        uint32_t next=p.generation.load(std::memory_order_relaxed)+1;if(next==0)next=1;p.generation.store(next,std::memory_order_relaxed);
        pageRetired.fetch_sub(1,std::memory_order_relaxed);pageUse.fetch_sub(1,std::memory_order_relaxed);
        const bool ok=freePages.push(id);assert(ok);(void)ok;
    }
    size_t collect(size_t budget)noexcept{
        size_t done=0;
        while(done<budget){size_t progress=0;
            for(int kind=0;kind<2&&done<budget;++kind){auto* bits=kind==0?retiredNodes.get():retiredPages.get();const size_t words=kind==0?nodeWords:pageWords;
                for(size_t word=0;word<words&&done<budget;++word){uint64_t pending=bits[word].exchange(0,std::memory_order_acquire);
                    while(pending&&done<budget){const unsigned bit=firstBit(pending);pending&=pending-1;const auto id=static_cast<uint32_t>(word*64+bit);if(kind==0)reclaimNode(id);else reclaimPage(id);++done;++progress;}
                    if(pending)bits[word].fetch_or(pending,std::memory_order_release);
                }
            }
            if(!progress)break;
        }
        return done;
    }
};

TapePool::TapePool(size_t pages,size_t nodes):reservation(std::make_unique<Reservation>(estimateBytes(pages,nodes))),impl(std::make_unique<Impl>(pages,nodes)){}
TapePool::~TapePool(){impl->collect(std::numeric_limits<size_t>::max());assert(impl->pageUse.load()==0&&impl->nodeUse.load()==0&&"TapePool must outlive its buffers");}
size_t TapePool::collect(size_t budget)noexcept{std::lock_guard<std::mutex> lock(impl->collectorMutex);return impl->collect(budget);}
TapePool::Stats TapePool::stats()const noexcept{
    Stats s;s.pageCapacity=impl->pageCount;s.nodeCapacity=impl->nodeCount;s.freePages=impl->freePages.available();s.freeNodes=impl->freeNodes.available();s.pagesInUse=impl->pageUse.load(std::memory_order_relaxed);s.nodesInUse=impl->nodeUse.load(std::memory_order_relaxed);s.retiredPages=impl->pageRetired.load(std::memory_order_relaxed);s.retiredNodes=impl->nodeRetired.load(std::memory_order_relaxed);
    s.allocatedBytes=sizeof(Impl)+s.pageCapacity*sizeof(Page)+s.nodeCapacity*sizeof(Node)+(s.pageCapacity+s.nodeCapacity+2)*sizeof(uint32_t)+(impl->pageWords+impl->nodeWords)*sizeof(std::atomic<uint64_t>);return s;
}
TapeBuffer::TapeBuffer(TapePool& p,uint64_t max)noexcept:pool(&p),limit(std::min(max,maxTapeFrames)){}
TapeBuffer::TapeBuffer(const TapeBuffer& other)noexcept:pool(other.pool),root(other.root),length(other.length),limit(other.limit){if(pool)pool->impl->retainNode(root);}
TapeBuffer::TapeBuffer(TapeBuffer&& other)noexcept:pool(other.pool),root(other.root),length(other.length),limit(other.limit){other.pool=nullptr;other.root=other.length=0;}
TapeBuffer& TapeBuffer::operator=(const TapeBuffer& other)noexcept{if(this!=&other){clear();pool=other.pool;root=other.root;length=other.length;limit=other.limit;if(pool)pool->impl->retainNode(root);}return *this;}
TapeBuffer& TapeBuffer::operator=(TapeBuffer&& other)noexcept{if(this!=&other){clear();pool=other.pool;root=other.root;length=other.length;limit=other.limit;other.pool=nullptr;other.root=other.length=0;}return *this;}
TapeBuffer::~TapeBuffer(){clear();}
void TapeBuffer::clear()noexcept{if(pool)pool->impl->releaseNode(root);root=length=0;}
float TapeBuffer::read(int channel,uint64_t frame)const noexcept{
    if(!pool||frame>=length||channel<0||channel>1)return 0;
    const Token p=pool->impl->findPage(root,frame/pageFrames);if(!p)return 0;return pool->impl->page(p).samples[static_cast<size_t>(channel)*pageFrames+static_cast<size_t>(frame%pageFrames)];
}
std::array<float,2> TapeBuffer::readStereo(uint64_t frame)const noexcept{
    if(!pool||frame>=length)return {0,0};
    const Token p=pool->impl->findPage(root,frame/pageFrames);if(!p)return {0,0};const auto& values=pool->impl->page(p).samples;const auto at=static_cast<size_t>(frame%pageFrames);return {values[at],values[pageFrames+at]};
}
bool TapeBuffer::writeStereo(uint64_t frame,float left,float right)noexcept{
    if(!pool||frame>=limit)return false;
    if(!pool->impl->write(root,frame,left,right))return false;
    length=std::max(length,frame+1);return true;
}
bool TapeBuffer::resizeLogical(uint64_t frames)noexcept{
    if(frames>limit||(!pool&&frames!=0))return false;
    if(frames==0){clear();return true;}if(frames<length&&!pool->impl->truncate(root,frames))return false;
    length=frames;return true;
}
}
