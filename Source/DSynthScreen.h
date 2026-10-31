#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
namespace field {
// Original vector control illustration from the supplied DSynth guide image.
// Secondary rows are conceptual: the audio model still has four DSynth macros.
void drawDSynthScreen(juce::Graphics&,juce::Rectangle<float>,const std::array<float,4>&,bool shifted,float phase);
}
