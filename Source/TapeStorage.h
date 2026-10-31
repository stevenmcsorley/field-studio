#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>

namespace field::tape {
inline constexpr uint32_t pageFrames=4096;
inline constexpr uint64_t maxTapeFrames=192000ull*360ull;

// One allocator/writer thread and one collect() worker per pool. Snapshots may
// be read, copied and released on any thread after safe publication. A snapshot
// must be made by the owner, not by racing a copy of the changing live buffer.
// Prepared edits on another thread must use another pool (or exclusive writer
// handoff). Every pool must outlive all buffers/snapshots that refer to it.
class TapePool {
public:
    struct Stats {
        size_t pageCapacity=0,nodeCapacity=0,freePages=0,freeNodes=0;
        size_t pagesInUse=0,nodesInUse=0,retiredPages=0,retiredNodes=0;
        size_t allocatedBytes=0;
    };
    // Allocates and physically touches all pages, nodes, queues and retire bits.
    // Construction, collection and destruction belong off the audio thread.
    TapePool(size_t pageCapacity,size_t nodeCapacity);
    static size_t memoryUsed() noexcept;
    static size_t memoryLimit() noexcept;
    static bool setMemoryLimit(size_t bytes); // control-only; cannot undercut live reservations
    static size_t estimateBytes(size_t pages,size_t nodes);
    ~TapePool();
    TapePool(const TapePool&)=delete;
    TapePool& operator=(const TapePool&)=delete;
    // Worker-only; budget counts reclaimed objects, not individual child refs.
    // Releases on other threads merely mark retirement bits and never free.
    size_t collect(size_t budget=std::numeric_limits<size_t>::max()) noexcept;
    Stats stats() const noexcept;
private:
    struct Reservation;
    struct Impl;
    std::unique_ptr<Reservation> reservation;
    std::unique_ptr<Impl> impl;
    friend class TapeBuffer;
};

class TapeBuffer {
public:
    TapeBuffer() noexcept=default;
    explicit TapeBuffer(TapePool&,uint64_t frameLimit=maxTapeFrames) noexcept;
    TapeBuffer(const TapeBuffer&) noexcept;
    TapeBuffer(TapeBuffer&&) noexcept;
    TapeBuffer& operator=(const TapeBuffer&) noexcept;
    TapeBuffer& operator=(TapeBuffer&&) noexcept;
    ~TapeBuffer();
    TapeBuffer snapshot() const noexcept { return *this; }
    float read(int channel,uint64_t frame) const noexcept;
    std::array<float,2> readStereo(uint64_t frame) const noexcept;
    // Atomic at the operation level: resource exhaustion leaves data/root/length
    // unchanged. No allocation/free/lock/retry loop. At most one32KiB page and
    // three fixed64-child nodes are copied on one write.
    bool writeStereo(uint64_t frame,float left,float right) noexcept;
    // Grow creates logical zero space. Shrink removes the tail via a bounded
    // tree walk and clears one partial page, so regrowth never revives old data.
    bool resizeLogical(uint64_t frames) noexcept;
    void clear() noexcept;
    uint64_t size() const noexcept { return length; }
    uint64_t capacityFrames() const noexcept { return limit; }
    bool empty() const noexcept { return length==0; }
    uint64_t rootIdentity() const noexcept { return root; } // Diagnostic only.
private:
    TapePool* pool=nullptr;
    uint64_t root=0,length=0,limit=maxTapeFrames;
};
}
