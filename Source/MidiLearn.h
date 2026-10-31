#pragma once
#include <array>
#include <cstddef>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace field {
enum class EncoderMode : unsigned { absolute,twosComplement,binaryOffset,signMagnitude };
struct MidiBinding { int channel=0,cc=-1;EncoderMode mode=EncoderMode::absolute;bool pickup=true,invert=false; };
// UI/host edits are atomic; only the audio consumer touches pickup/accumulator
// state. No allocation, listener notification, lock or unbounded retry loop.
template<size_t Count> class MidiLearn {
    static_assert(Count<256);
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
public:
    MidiLearn(){for(auto& item:slots)item.store(0);}
    static bool valid(MidiBinding b) noexcept {return b.channel>=0&&b.channel<=16&&b.cc>=0&&b.cc<120&&b.cc!=64&&(unsigned)b.mode<4;}
    MidiBinding binding(size_t target) const noexcept {return target<Count?decode(slots[target].load(std::memory_order_acquire)):MidiBinding{};}
    bool assign(size_t target,MidiBinding b) noexcept {if(target>=Count||!valid(b))return false;slots[target].store(pack(b),std::memory_order_release);return true;}
    void clear(size_t target) noexcept {if(target<Count)slots[target].store(pack({}),std::memory_order_release);}
    void clearAll() noexcept {cancel();for(size_t i=0;i<Count;++i)clear(i);}
    bool arm(size_t target,EncoderMode mode,bool pickup,bool invert=false) noexcept {
        if(target>=Count||(unsigned)mode>3)return false;
        const uint64_t expected=slots[target].load(std::memory_order_acquire);
        const uint64_t ticket=(learnSerial.fetch_add(1,std::memory_order_relaxed)+1)&0x7ffff;
        pending.store((expected<<32)|(ticket<<13)|((uint64_t)invert<<12)|((uint64_t)pickup<<11)|((uint64_t)mode<<9)|(target+1),std::memory_order_release);return true;
    }
    void cancel() noexcept {pending.store(0,std::memory_order_release);}
    int learningTarget() const noexcept {return (int)(pending.load(std::memory_order_acquire)&0x1ff)-1;}
    template<class Get,class Set> bool process(int channel,int cc,int value,Get&& get,Set&& set) noexcept {
        if(channel<1||channel>16||cc<0||cc>=120||cc==64||value<0||value>127)return false;
        uint64_t request=pending.load(std::memory_order_acquire);
        if(request&&pending.compare_exchange_strong(request,0,std::memory_order_acq_rel)){
            const size_t target=(request&0x1ff)-1;uint32_t expected=(uint32_t)(request>>32);
            MidiBinding b{channel,cc,(EncoderMode)((request>>9)&3),((request>>11)&1)!=0,((request>>12)&1)!=0};
            const uint32_t next=pack(b);slots[target].compare_exchange_strong(expected,next,std::memory_order_acq_rel);return true;
        }
        bool consumed=false;
        for(size_t target=0;target<Count;++target){const auto config=slots[target].load(std::memory_order_acquire);const auto b=decode(config);
            if(b.cc!=cc||(b.channel!=0&&b.channel!=channel))continue;consumed=true;auto& s=state[target];const float current=std::clamp(get(target),0.f,1.f);
            if(s.config!=config){s={};s.config=config;s.lastInput=-1;s.accumulator=current;s.lastApplied=current;}
            if(std::abs(current-s.lastApplied)>.0001f){s.latched=false;s.accumulator=current;}
            float next=current;
            if(b.mode==EncoderMode::absolute){const float input=b.invert?1-value/127.f:value/127.f;
                if(!b.pickup||s.latched||std::abs(input-current)<=1.5f/127.f||(s.lastInput>=0&&(s.lastInput-current)*(input-current)<=0)){s.latched=true;next=input;}
                s.lastInput=input;if(!s.latched){s.lastApplied=current;continue;}
            }else {int delta=0;if(b.mode==EncoderMode::twosComplement)delta=value==64?0:value<64?value:value-128;
                else if(b.mode==EncoderMode::binaryOffset)delta=value-64;
                else delta=value==64?0:value<64?value:64-value;
                if(b.invert)delta=-delta;if(delta==0){s.lastApplied=current;continue;}next=std::clamp(s.accumulator+delta/127.f,0.f,1.f);s.accumulator=next;
            }
            set(target,next);s.lastApplied=std::clamp(get(target),0.f,1.f);
        }return consumed;
    }
private:
    // revision14 | invert1 | pickup1 | spare1 | mode2 | channel5 | CC+1 8.
    uint32_t pack(MidiBinding b) noexcept {return ((serial.fetch_add(1,std::memory_order_relaxed)+1)&0x3fff)<<18|((uint32_t)b.invert<<17)|((uint32_t)b.pickup<<16)|((uint32_t)b.mode<<13)|((uint32_t)b.channel<<8)|(uint32_t)(b.cc+1);}
    static MidiBinding decode(uint32_t bits) noexcept {return {(int)((bits>>8)&31),(int)(bits&255)-1,(EncoderMode)((bits>>13)&3),((bits>>16)&1)!=0,((bits>>17)&1)!=0};}
    struct Runtime {uint32_t config=0;float lastInput=-1,accumulator=0,lastApplied=0;bool latched=false;};
    std::array<std::atomic<uint32_t>,Count> slots;
    std::array<Runtime,Count> state{};
    std::atomic<uint32_t> serial{0},learnSerial{0};
    std::atomic<uint64_t> pending{0};
};
}
