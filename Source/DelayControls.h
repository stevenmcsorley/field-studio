#pragma once
#include <algorithm>
#include <cmath>
namespace field {
inline float delayRangeMs(float value){constexpr float ranges[]{62.5f,125.f,250.f,500.f,750.f,1000.f};return ranges[std::clamp((int)(std::clamp(value,0.f,1.f)*5.999f),0,5)];}
inline double delaySeconds(float range,float speed){return delayRangeMs(range)*.001*std::exp2((.5-std::clamp(speed,0.f,1.f))*2);}
}
