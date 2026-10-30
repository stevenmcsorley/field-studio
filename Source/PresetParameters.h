#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace field::preset {
struct ParameterValue {
    juce::String id;
    float value=0;
};
struct ControlPosition {
    juce::String group;
    int index=0;
    double raw=0,minimum=0,maximum=32767;
    float position=0;
    juce::String targetParameter;
    juce::String method;
};
struct ParameterTranslation {
    std::vector<ParameterValue> parameterValues;
    std::vector<ControlPosition> controls;
    juce::StringArray mapped,unsupported;
    juce::String sourceProfile;
    int sourceEffect=-1,sourceLfo=-1;
    bool effectActive=false,lfoActive=false;
    bool exactSound=false;
};
// Control-thread only. Positions are translated from observed Field raw travel
// into independently implemented software DSP controls, NOT calibrated sound
// parameters. No claim of hardware DSP/ADSR timing/gain-law equivalence follows.
// Source JSON is never modified. Sample markers belong to PresetMapping and
// DBox per-pad data is not treated as a global melodic macro array.
// Invalid present numeric arrays fail without changing output. Missing and
// unknown fields are reported as unsupported rather than silently guessed.
bool translateParameters(const juce::var&,ParameterTranslation&,juce::String& error);
}
