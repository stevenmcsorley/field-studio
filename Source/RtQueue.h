#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace field {
// Exactly one producer and one consumer. Fixed slots, no waits or retry loops.
// 63-bit sequence positions allow a panic fence to discard an old generation
// without discarding newer events or rewinding past already consumed events.
template<class T, size_t Capacity> class RtQueue {
    static_assert(Capacity>1 && (Capacity&(Capacity-1))==0,"power-of-two capacity required");
    static_assert(std::is_trivially_copyable<T>::value,"fixed POD queue required");
    static_assert(std::atomic<uint64_t>::is_always_lock_free,"lock-free indices required");
public:
    static constexpr uint64_t positionMask=(uint64_t{1}<<63)-1;
    bool push(const T& value) noexcept {
        const auto head=write.load(std::memory_order_relaxed);
        if(((head-read.load(std::memory_order_acquire))&positionMask)>=Capacity)return false;
        slots[(size_t)(head&(Capacity-1))]=value;
        write.store((head+1)&positionMask,std::memory_order_release);return true;
    }
    bool pop(T& value) noexcept {uint64_t ignored=0;return pop(value,ignored);}
    bool pop(T& value,uint64_t& position) noexcept {
        const auto tail=read.load(std::memory_order_relaxed);
        if(tail==write.load(std::memory_order_acquire))return false;
        value=slots[(size_t)(tail&(Capacity-1))];position=tail;
        read.store((tail+1)&positionMask,std::memory_order_release);return true;
    }
    uint64_t producerPosition() const noexcept {return write.load(std::memory_order_relaxed);}
    // Consumer only. An older fence can never resurrect consumed entries.
    void discardThrough(uint64_t fence) noexcept {
        const auto tail=read.load(std::memory_order_relaxed),head=write.load(std::memory_order_acquire);
        if(((fence-tail)&positionMask)<=((head-tail)&positionMask))read.store(fence&positionMask,std::memory_order_release);
    }
    void discardPending() noexcept {discardThrough(write.load(std::memory_order_acquire));}
private:
    std::array<T,Capacity> slots{};
    alignas(64) std::atomic<uint64_t> write{0};
    alignas(64) std::atomic<uint64_t> read{0};
};
}
