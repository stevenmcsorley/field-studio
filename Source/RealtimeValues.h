#pragma once
#include <array>
#include <cstddef>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
namespace field {
// Physical notification snapshots are atomically published and separately
// acknowledged. The parameter itself remains the single value authority;
// acknowledging an old snapshot never writes back into that parameter.
template<std::size_t Count> class RealtimeValues {
    static_assert(std::atomic<uint64_t>::is_always_lock_free,"lock-free parameter cells required");
public:
    struct Snapshot {uint64_t packed=0;float value=0;};
    void initialize(std::size_t key,float value) noexcept {const auto packed=pack(value);cells[key].current.store(packed);cells[key].mirrored.store(packed);}
    float load(std::size_t key) const noexcept {return unpack(cells[key].current.load(std::memory_order_acquire));}
    void fromAudio(std::size_t key,float value) noexcept {cells[key].current.store(pack(value),std::memory_order_release);}
    uint64_t version(std::size_t key) const noexcept {return cells[key].current.load(std::memory_order_acquire);}
    // Capture expected BEFORE reading the native parameter. One failed CAS
    // leaves a concurrent audio notification pending; never spin in a listener.
    bool fromControlIfUnchanged(std::size_t key,uint64_t expected,float value) noexcept {const auto packed=pack(value);if(!cells[key].current.compare_exchange_strong(expected,packed,std::memory_order_acq_rel,std::memory_order_acquire))return false;cells[key].mirrored.store(packed,std::memory_order_release);return true;}
    // Exclusive lifecycle synchronization (audio writer quiescent).
    void fromControl(std::size_t key,float value) noexcept {const auto packed=pack(value);cells[key].current.store(packed,std::memory_order_release);cells[key].mirrored.store(packed,std::memory_order_release);}
    bool pending(std::size_t key,Snapshot& snapshot) const noexcept {snapshot.packed=cells[key].current.load(std::memory_order_acquire);snapshot.value=unpack(snapshot.packed);return snapshot.packed!=cells[key].mirrored.load(std::memory_order_acquire);}
    void acknowledge(std::size_t key,const Snapshot& snapshot) noexcept {cells[key].mirrored.store(snapshot.packed,std::memory_order_release);}
private:
    uint64_t pack(float value) noexcept {if(!std::isfinite(value))value=0;uint32_t bits=0;std::memcpy(&bits,&value,sizeof(bits));return (uint64_t(serial.fetch_add(1,std::memory_order_relaxed))<<32)|bits;}
    static float unpack(uint64_t value) noexcept {const auto bits=static_cast<uint32_t>(value);float result;std::memcpy(&result,&bits,sizeof(result));return result;}
    struct Cell {std::atomic<uint64_t> current{0},mirrored{0};};
    std::array<Cell,Count> cells{};
    std::atomic<uint32_t> serial{1};
};
}
