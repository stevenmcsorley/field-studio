#pragma once
#include "PresetFormat.h"
#include <array>
namespace field::preset {
struct ImportedPad {
    float start=0,end=1,tune=0,level=0,pan=0;
    bool reverse=false;
    int mode=0;
};
struct SampleMapping {
    bool sampled=false,drums=false,hasMarkers=false,hasRoot=false;
    std::array<float,4> markers{{0,0,1,1}};
    float rootNote=60;
    std::array<ImportedPad,24> pads;
    juce::String profile;
    juce::StringArray warnings;
};
// Converts only documented sample positions/tuning and corroborated kit fields.
// Original synth DSP/ADSR/FX/LFO integers are deliberately not approximated here.
// Marker profiles are explicit: legacy mono12-second fixed-point, stereo20-second
// Q31, and melodic sampler6-second Q15. No duration-dependent scale guessing.
bool sampleMapping(const juce::var&,double durationSeconds,SampleMapping&,juce::String& error);
}
