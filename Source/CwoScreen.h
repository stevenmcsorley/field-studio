#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
namespace field {
void drawCwoScreen(juce::Graphics&,juce::Rectangle<float>,const std::array<float,4>&,float phase);
}
