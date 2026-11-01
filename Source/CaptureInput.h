#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <memory>
#include "CaptureClockBridge.h"
// One producer (capture device), one consumer (instrument audio callback).
class CaptureRing {
public:
 static constexpr size_t capacity=65536;
 void push(const float* stereo,size_t count) noexcept {auto w=write.load(std::memory_order_relaxed),r=read.load(std::memory_order_acquire);for(size_t i=0;i<count;++i){if(w-r>=capacity){dropped.fetch_add(count-i,std::memory_order_relaxed);break;}data[w%capacity]={stereo?stereo[i*2]:0,stereo?stereo[i*2+1]:0};++w;}write.store(w,std::memory_order_release);}
 size_t pop(float* left,float* right,size_t count) noexcept {auto r=read.load(std::memory_order_relaxed),w=write.load(std::memory_order_acquire);size_t copied=0;for(size_t i=0;i<count;++i){if(r<w){left[i]=data[r%capacity][0];right[i]=data[r%capacity][1];++r;++copied;}else left[i]=right[i]=0;}read.store(r,std::memory_order_release);return copied;}
 size_t available() const noexcept {const auto r=read.load(std::memory_order_acquire),w=write.load(std::memory_order_acquire);return w>=r?(size_t)(w-r):0;}
 size_t popResampled(float* left,float* right,size_t count,double ratio,double& phase) noexcept {
  const auto r=read.load(std::memory_order_relaxed),w=write.load(std::memory_order_acquire);const size_t available=(size_t)(w-r);double cursor=phase;size_t copied=0;
  for(size_t i=0;i<count;++i){const size_t at=(size_t)cursor;const float fraction=(float)(cursor-at);if(at>=available||(fraction>0&&at+1>=available)){left[i]=right[i]=0;continue;}const auto a=data[(r+at)%capacity],b=data[(r+std::min(at+1,available-1))%capacity];left[i]=a[0]+(b[0]-a[0])*fraction;right[i]=a[1]+(b[1]-a[1])*fraction;cursor+=ratio;++copied;}
  const auto consumed=std::min((size_t)cursor,available);phase=cursor-consumed;read.store(r+consumed,std::memory_order_release);return copied;
 }
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
 void read(float* l,float* r,int frames) noexcept {bridge.read(ring,l,r,(size_t)frames);}
 CaptureRing ring;CaptureClockBridge bridge;
private:
 struct Impl;std::unique_ptr<Impl> impl;std::atomic<bool> running{false};
};
