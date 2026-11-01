#pragma once
#include <array>
#include <atomic>
#include <type_traits>
namespace field {
// One producer and one consumer. Neither waits; the consumer owns its read slot
// until its next exchange. Superseded, unread states are reused by the producer.
// Producer ownership may be handed off only under external synchronization.
template<class T> class RealtimeSnapshot {
 static_assert(std::is_trivially_destructible_v<T>,"Snapshots must not own heap resources");
public:
 void publish(const T& state) noexcept {slots[writer]=state;writer=middle.exchange(writer|dirty,std::memory_order_acq_rel)&3u;}
 T read() noexcept {if(middle.load(std::memory_order_acquire)&dirty)reader=middle.exchange(reader,std::memory_order_acq_rel)&3u;return slots[reader];}
private:
 static constexpr unsigned dirty=4;std::array<T,3> slots{};unsigned writer=0,reader=2;std::atomic<unsigned> middle{1};
};
}
