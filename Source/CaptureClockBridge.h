#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
// Bounded, gently adaptive capture-clock matching. The capture device and the
// output device need not share a clock. All interpolation happens on the consumer.
class CaptureClockBridge {
public:
 void reset(int period=256) noexcept {periodFrames=std::max(1,period);phase=0;ratio=1;primed=false;underruns=0;recoveries=0;ratioReading=1;}
 template<class Ring>void read(Ring& ring,float* left,float* right,size_t count) noexcept {
  if(count==0)return;
  const size_t target=std::max(count+(size_t)periodFrames,(size_t)periodFrames*2),ceiling=target+std::max(count,(size_t)periodFrames)*2;
  if(ring.available()>ceiling){ring.trim(target);phase=0;ratio=1;++recoveries;}
  if(!primed){if(ring.available()<target){std::fill_n(left,count,0);std::fill_n(right,count,0);return;}primed=true;}
  const double error=((double)ring.available()-target)/target;
  ratio+=(1+std::clamp(error*.002,-.005,.005)-ratio)*.02;
  const auto got=ring.popResampled(left,right,count,ratio,phase);underruns.fetch_add(count-got,std::memory_order_relaxed);ratioReading=(float)ratio;
  if(got==0){primed=false;phase=0;}
 }
 std::atomic<uint64_t> underruns{0},recoveries{0};std::atomic<float> ratioReading{1};
private:
 int periodFrames=256;double phase=0,ratio=1;bool primed=false;
};
