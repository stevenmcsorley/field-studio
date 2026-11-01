#pragma once
#include <array>
#include <atomic>
#include <utility>

namespace field {
// Latest-value, single-producer/single-consumer ownership exchange. T contains
// retained handles only; the control-side registry MUST pin every referenced
// heap owner before publication. Replacing a slot on audio then only decrements
// references and retires COW tree nodes; it can never destroy a heap owner.
// Root/pool/sample registries collect their last reference on the control side.
template<class T,unsigned Count=3> class OwnedSnapshots {
 static_assert(Count==3);
public:
 void reset(){for(auto& slot:slots)slot=T{};writer=0;reader=2;middle.store(1,std::memory_order_relaxed);}
 template<class Fill> bool publish(Fill&& fill) noexcept {fill(slots[writer]);writer=middle.exchange(writer|dirty,std::memory_order_acq_rel)&3u;return true;}
 template<class Consume> void consume(Consume&& consume){if(middle.load(std::memory_order_acquire)&dirty){reader=middle.exchange(reader,std::memory_order_acq_rel)&3u;consume(static_cast<const T&>(slots[reader]));}}
private:
 static constexpr unsigned dirty=4;
 std::array<T,3> slots{};unsigned writer=0,reader=2;std::atomic<unsigned> middle{1};
};
}
