#pragma once
#include <algorithm>
#include <cmath>
namespace field {
// UI-only motion. Rates are an original interpolation, not measured firmware
// timing. Integrating elapsed time avoids phase jumps when a knob changes.
struct CwoAnimation {
    float frequencyTurns=0,flowTurns=0;
    void advance(float frequency,float delay,double elapsed) noexcept {
        if(!std::isfinite(elapsed)||elapsed<=0)return;
        const float dt=(float)std::min(elapsed,.2);
        frequency=std::isfinite(frequency)?std::clamp(frequency,0.f,1.f):0;
        delay=std::isfinite(delay)?std::clamp(delay,0.f,1.f):0;
        frequencyTurns+=dt*(.08f+frequency*2.2f);frequencyTurns-=std::floor(frequencyTurns);
        flowTurns+=dt*(.15f+(1-delay)*1.85f);flowTurns-=std::floor(flowTurns);
    }
};
struct DelayCircle {float x,y,radius;};
// Range-linked nested circles and centre marker are observed in the field
// demonstration. Absolute geometry below is original, fitted to our LCD.
inline DelayCircle delayCircle(float milliseconds){
    const float position=std::clamp(std::log2(std::max(62.5f,milliseconds)/62.5f)/4.f,0.f,1.f);
    const float radius=40.f+78.f*position;
    return {310.f+(118.f-radius)*.79f,149.f+(118.f-radius)*.61f,radius};
}
}
