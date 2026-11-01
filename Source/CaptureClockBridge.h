#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstddef>
// Bounded, gently adaptive capture-clock matching. The capture device and the
// output device need not share a clock. All interpolation happens on the consumer.
class CaptureClockBridge {
public:
 void reset(int period=256) noexcept {periodFrames=std::max(1,period);phase=0;ratio=1;primed=false;last[0]=last[1]=0;fadeRemaining=0;silenceRemaining=0;underruns=0;recoveries=0;ratioReading=1;}
 template<class Ring>void read(Ring& ring,float* left,float* right,size_t count) noexcept {
  if(count==0)return;
  const size_t target=std::max(count+(size_t)periodFrames,(size_t)periodFrames*2),ceiling=target+std::max(count,(size_t)periodFrames)*2;
  if(ring.available()>ceiling){ring.trim(target);phase=0;ratio=1;beginTransition();++recoveries;}
  if(!primed){if(ring.available()<target){silence(left,right,count);return;}primed=true;beginTransition();}
  const double error=((double)ring.available()-target)/target;
  ratio+=(1+std::clamp(error*.002,-.005,.005)-ratio)*.02;
  const auto got=ring.popResampled(left,right,count,ratio,phase);underruns.fetch_add(count-got,std::memory_order_relaxed);ratioReading=(float)ratio;
  for(size_t i=0;i<got;++i){if(fadeRemaining>0){const float mix=(float)(transitionFrames-fadeRemaining+1)/transitionFrames;left[i]=anchor[0]+(left[i]-anchor[0])*mix;right[i]=anchor[1]+(right[i]-anchor[1])*mix;--fadeRemaining;}last[0]=left[i];last[1]=right[i];}
  if(got<count){silence(left+got,right+got,count-got);primed=false;phase=0;fadeRemaining=0;}
  else silenceRemaining=0;
 }
 std::atomic<uint64_t> underruns{0},recoveries{0};std::atomic<float> ratioReading{1};
private:
 static constexpr int transitionFrames=64;
 void beginTransition() noexcept {anchor[0]=last[0];anchor[1]=last[1];fadeRemaining=transitionFrames;silenceRemaining=0;}
 void silence(float* left,float* right,size_t count) noexcept {
  if(silenceRemaining==0&&(last[0]!=0||last[1]!=0))silenceRemaining=transitionFrames;
  for(size_t i=0;i<count;++i){if(silenceRemaining>0){const float gain=(float)(silenceRemaining-1)/silenceRemaining;last[0]*=gain;last[1]*=gain;--silenceRemaining;}else last[0]=last[1]=0;left[i]=last[0];right[i]=last[1];}
 }
 float last[2]{},anchor[2]{};int fadeRemaining=0,silenceRemaining=0;
 int periodFrames=256;double phase=0,ratio=1;bool primed=false;
};
