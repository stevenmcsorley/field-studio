#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace field {
struct LfoScreenState {
 juce::String parameterLabel;int focus=-1;
 std::array<float,8> randomHistory{};
 int type=0,target=1,waveform=0,source=0,division=4;
 float rate=1,amount=0,envelope=0,pitch=0,volume=0,velocityVolume=1,phase=0,modulation=0,manual=.5f;
 bool randomAll=true,enabled=true,sync=false,retrigger=false,shifted=false,reference=true;
 std::array<int,4> midiTargets{{1,2,8,12}};std::array<float,4> midiAmounts{},midiValues{};
};
void drawLfoScreen(juce::Graphics&,juce::Rectangle<float>,const LfoScreenState&);
}
