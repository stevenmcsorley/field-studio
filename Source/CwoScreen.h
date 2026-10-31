#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "EffectAnimation.h"
namespace field {
void drawCwoScreen(juce::Graphics&,juce::Rectangle<float>,const std::array<float,4>&,const CwoAnimation& motion);
}
