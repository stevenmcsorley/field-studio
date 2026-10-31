#pragma once
#include <array>
#include <cmath>
namespace field {
// Original variable-clock colour model. Slow clocks prefilter then hold
// samples; faster clocks retain the full host-rate signal. No heap storage.
class DelayColor {
public:
    void reset() noexcept {low1={};low2={};held={};clock=0;started=false;}
    std::array<float,2> process(float l,float r,float ratio) noexcept {
        if(ratio>=1){low1={{l,r}};low2=held=low1;clock=0;started=true;return held;}
        const float coefficient=1-std::exp(-6.28318530717958647692f*.42f*ratio);
        const float input[]{l,r};for(size_t channel=0;channel<2;++channel){low1[channel]+=coefficient*(input[channel]-low1[channel]);low2[channel]+=coefficient*(low1[channel]-low2[channel]);}
        clock+=ratio;if(!started||clock>=1){held=low2;started=true;clock-=std::floor(clock);}return held;
    }
private:
    std::array<float,2> low1{},low2{},held{};
    float clock=0;bool started=false;
};
}
