#pragma once
#include <array>
#include <algorithm>
#include <cmath>
namespace field {
// Original windowed-FIR analytic frequency shifter. Fixed storage throughout;
// prepare() builds coefficients off audio. Not a recovered hardware algorithm.
class CwoShifter {
public:
    void prepare(double rate){
        sr=rate;phase=0;write=0;left.fill(0);right.fill(0);
        constexpr double pi=3.14159265358979323846;
        for(int i=0;i<65;++i){const int k=i-32;coefficients[(size_t)i]=k%2?static_cast<float>(2./(pi*k)*(.54-.46*std::cos(2*pi*i/64))):0.f;}
    }
    std::array<float,2> process(float l,float r,float frequency,float sideband) noexcept {
        left[write]=l;right[write]=r;float imaginaryL=0,imaginaryR=0;
        for(size_t tap=1;tap<65;tap+=2){const size_t at=(write+65-tap)%65;imaginaryL+=left[at]*coefficients[tap];imaginaryR+=right[at]*coefficients[tap];}
        const size_t centre=(write+65-32)%65;const float realL=left[centre],realR=right[centre];
        constexpr double tau=6.28318530717958647692;
        const float cosine=(float)std::cos(tau*phase),sine=(float)std::sin(tau*phase);
        phase+=frequency/sr;phase-=std::floor(phase);write=(write+1)%65;
        // Left endpoint is the lower sideband, right endpoint the upper;
        // the midpoint contains both. There is no unshifted-source endpoint.
        const float quadrature=1-2*sideband;
        return {{realL*cosine+imaginaryL*sine*quadrature,realR*cosine+imaginaryR*sine*quadrature}};
    }
private:
    std::array<float,65> left{},right{},coefficients{};
    size_t write=0;
    double sr=44100,phase=0;
};
}
