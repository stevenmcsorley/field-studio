#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace field {
// Original vector rendering from the supplied Dimension guide screenshot.
// The XY trace is a control illustration, not sampled output telemetry.
void drawDimensionScreen(juce::Graphics&,juce::Rectangle<float>,float waveform,float stereo,float cutoff,float resonance,double sampleRate,float phase);
}
