#pragma once
#include "RtQueue.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace field {
struct KeyboardEvent { uint8_t status=0,data1=0,data2=0; };
// Message-thread producer, audio-thread consumer. UI key state and received key
// state have separate atomic storage; the callback never calls a UI listener.
class RtKeyboard {
    static_assert(std::atomic<uint32_t>::is_always_lock_free,"lock-free key bits required");
public:
    RtKeyboard() noexcept {for(auto& keys:guiKeys)keys.store(0,std::memory_order_relaxed);clearIncoming();}
    bool noteOn(int channel,int note,float velocity) noexcept {
        if(!valid(channel,note)||!std::isfinite(velocity))return false;
        if(velocity<=0)return noteOff(channel,note,0);
        guiKeys[(size_t)note].fetch_or(1u<<(channel-1),std::memory_order_relaxed);
        return send({(uint8_t)(0x90+channel-1),(uint8_t)note,(uint8_t)std::clamp((int)std::lround(std::clamp(velocity,0.f,1.f)*127),1,127)});
    }
    bool noteOff(int channel,int note,float) noexcept {
        if(!valid(channel,note))return false;
        guiKeys[(size_t)note].fetch_and(~(1u<<(channel-1)),std::memory_order_relaxed);
        return send({(uint8_t)(0x80+channel-1),(uint8_t)note,0});
    }
    void allNotesOff(int channel) noexcept {
        if(channel==0){for(auto& keys:guiKeys)keys.store(0,std::memory_order_relaxed);requestPanic();return;}
        if(channel<1||channel>16)return;
        for(auto& keys:guiKeys)keys.fetch_and(~(1u<<(channel-1)),std::memory_order_relaxed);
        send({(uint8_t)(0xb0+channel-1),123,0});
    }
    bool isGuiNoteOn(int channel,int note) const noexcept {return valid(channel,note)&&(guiKeys[(size_t)note].load(std::memory_order_relaxed)&(1u<<(channel-1)))!=0;}
    bool isNoteOn(int channel,int note) const noexcept {return valid(channel,note)&&isNoteOnForChannels(1u<<(channel-1),note);}
    bool isNoteOnForChannels(int channels,int note) const noexcept {
        if(note<0||note>127)return false;
        return ((guiKeys[(size_t)note].load(std::memory_order_relaxed)|inputKeys[(size_t)note].load(std::memory_order_relaxed))&(uint32_t)channels&0xffffu)!=0;
    }
    // Audio side; called only for original incoming MIDI, never GUI echoes.
    void observe(uint8_t status,uint8_t data1,uint8_t data2) noexcept {
        const auto kind=status&0xf0u;const uint32_t bit=1u<<(status&15u);
        if((kind==0x90u||kind==0x80u)&&data1<128){auto& keys=inputKeys[data1];if(kind==0x90u&&data2>0)keys.fetch_or(bit,std::memory_order_relaxed);else keys.fetch_and(~bit,std::memory_order_relaxed);}
        else if(kind==0xb0u&&(data1==123||data1==120))for(auto& keys:inputKeys)keys.fetch_and(~bit,std::memory_order_relaxed);
    }
    // A zero status is an ordered global-panic command. Check the panic fence
    // AFTER popping too: a producer may publish panic+note during this call.
    bool pop(KeyboardEvent& event) noexcept {
        if(const auto request=panicFence.exchange(0,std::memory_order_acq_rel))return emitPanic(request,event);
        if(hasDeferred){event=deferred;hasDeferred=false;return true;}
        KeyboardEvent next;uint64_t position=0;
        if(!events.pop(next,position)){if(const auto request=panicFence.exchange(0,std::memory_order_acq_rel))return emitPanic(request,event);return false;}
        if(const auto request=panicFence.exchange(0,std::memory_order_acq_rel)){const auto fence=request&Queue::positionMask;if(!older(position,fence)){deferred=next;deferredPosition=position;hasDeferred=true;}return emitPanic(request,event);}
        event=next;return true;
    }
    void clearIncoming() noexcept {for(auto& keys:inputKeys)keys.store(0,std::memory_order_relaxed);}
    uint32_t overflowCount() const noexcept {return overflows.load(std::memory_order_relaxed);}
private:
    static bool valid(int channel,int note) noexcept {return channel>=1&&channel<=16&&note>=0&&note<128;}
    bool send(KeyboardEvent event) noexcept {
        if(events.push(event))return true;
        overflows.fetch_add(1,std::memory_order_relaxed);for(auto& keys:guiKeys)keys.store(0,std::memory_order_relaxed);requestPanic();return false;
    }
    using Queue=RtQueue<KeyboardEvent,512>;
    void requestPanic() noexcept {panicFence.store((uint64_t{1}<<63)|events.producerPosition(),std::memory_order_release);}
    static bool older(uint64_t position,uint64_t fence) noexcept {const auto distance=(fence-position)&Queue::positionMask;return distance!=0&&distance<(uint64_t{1}<<62);}
    bool emitPanic(uint64_t request,KeyboardEvent& event) noexcept {const auto fence=request&Queue::positionMask;if(hasDeferred&&older(deferredPosition,fence))hasDeferred=false;events.discardThrough(fence);event={0,0,0};return true;}
    Queue events;
    KeyboardEvent deferred{};
    uint64_t deferredPosition=0;
    bool hasDeferred=false; // audio consumer only
    std::array<std::atomic<uint32_t>,128> guiKeys{},inputKeys{};
    std::atomic<uint64_t> panicFence{0};
    std::atomic<uint32_t> overflows{0};
};
}
