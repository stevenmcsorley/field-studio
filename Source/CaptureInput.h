#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <memory>
// One producer (capture device), one consumer (instrument audio callback).
class CaptureRing {
public:
 static constexpr size_t capacity=65536;
 void push(const float* stereo,size_t count) noexcept {auto w=write.load(std::memory_order_relaxed),r=read.load(std::memory_order_acquire);for(size_t i=0;i<count;++i){if(w-r>=capacity){dropped.fetch_add(count-i,std::memory_order_relaxed);break;}data[w%capacity]={stereo?stereo[i*2]:0,stereo?stereo[i*2+1]:0};++w;}write.store(w,std::memory_order_release);}
 size_t pop(float* left,float* right,size_t count) noexcept {auto r=read.load(std::memory_order_relaxed),w=write.load(std::memory_order_acquire);size_t copied=0;for(size_t i=0;i<count;++i){if(r<w){left[i]=data[r%capacity][0];right[i]=data[r%capacity][1];++r;++copied;}else left[i]=right[i]=0;}read.store(r,std::memory_order_release);return copied;}
 void trim(size_t maximum) noexcept {const auto w=write.load(std::memory_order_acquire),r=read.load(std::memory_order_relaxed);if(w-r>maximum){dropped.fetch_add(w-r-maximum,std::memory_order_relaxed);read.store(w-maximum,std::memory_order_release);}}
 void clear() noexcept {read.store(write.load(std::memory_order_acquire),std::memory_order_release);}
 std::atomic<uint64_t> dropped{0};
private:
 std::array<std::array<float,2>,capacity> data{};std::atomic<uint64_t> read{0},write{0};
};
class CaptureInput {
public:
 CaptureInput();~CaptureInput();
 juce::StringArray devices();
 bool start(int index,double rate,juce::String& error);
 void stop();
 bool active()const noexcept{return running.load();}
 void read(float* l,float* r,int frames) noexcept {ring.trim(std::max((size_t)8192,(size_t)frames*8));ring.pop(l,r,(size_t)frames);}
 CaptureRing ring;
private:
 struct Impl;std::unique_ptr<Impl> impl;std::atomic<bool> running{false};
};
