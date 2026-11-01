#pragma once
#include "MixerScreen.h"
namespace field {
inline void drawVocoderScreen(juce::Graphics& g,juce::Rectangle<float> bounds,const std::array<float,4>& values,const std::array<float,16>& levels,float input,juce::String source){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,bounds);const auto blue=mixerColours[0],ochre=mixerColours[1],grey=mixerColours[2],red=mixerColours[3];const juce::Colour purple(0xff71345e);
 mixerText(g,"WAVEFORM",{106,16,170,30},22,blue);mixerText(g,"FORMANT",{107,248,166,31},23,ochre);mixerText(g,"MIX",{372,39,90,26},22,red);mixerText(g,"BANDS",{352,226,129,29},23,grey);
 const float jaw=input*12;juce::Path head;head.startNewSubPath(149,237);head.lineTo(154,197);head.cubicTo(106,157,116,91,147,78);head.cubicTo(177,60,231,65,238,84);head.cubicTo(257,86,236,104,241,122);head.lineTo(259,155);head.lineTo(246,162);head.lineTo(247,176+jaw);head.cubicTo(229,193,215,194,198,182);head.lineTo(199,237);head.closeSubPath();g.setColour(ochre);g.strokePath(head,juce::PathStrokeType(2));
 g.setColour(blue.withAlpha(.4f));g.fillRoundedRectangle({158,85,30,30},4);mixerText(g,juce::String(std::min(7,(int)(values[0]*8))),{158,84,30,32},26,blue);
 g.setColour(red);g.drawEllipse(168,127,19,27,2);g.fillEllipse(180,140,7,7);g.fillEllipse(222,164+jaw,7,7);g.setColour(ochre);g.drawLine(221,128,228,126,2);g.drawLine(183,222,183,164,2);g.fillEllipse(179,218,8,8);mixerText(g,juce::String((values[1]-.5f)*2,2),{247,136,60,30},22,ochre);
 juce::Path arc;arc.addCentredArc(187,136,67+values[1]*13,67,0,-2.6f,-.15f,true);g.setColour(red);g.strokePath(arc,juce::PathStrokeType(2));
 juce::Path small;small.startNewSubPath(398,171);small.cubicTo(357,174,348,120,370,108);small.cubicTo(389,90,429,97,438,115);small.lineTo(456,144);small.lineTo(444,151);small.cubicTo(436,173,421,180,398,171);g.setColour(ochre);g.strokePath(small,juce::PathStrokeType(2));
 juce::Path mix;mix.addCentredArc(400,141,56,53,0,-2.8f,-2.8f+values[3]*5.6f,true);g.setColour(red);g.strokePath(mix,juce::PathStrokeType(3));g.fillEllipse(394,137,8,8);mixerText(g,juce::String(4+(int)std::round(values[2]*12)),{359,124,33,35},26,grey);
 const int count=4+(int)std::round(values[2]*12);for(int i=0;i<count;++i){float angle=-.9f+i*1.8f/std::max(1,count-1);float radius=69+std::min(1.f,levels[(size_t)i]*25)*28;g.setColour(purple);g.drawLine(420+std::cos(angle)*65,146+std::sin(angle)*65,420+std::cos(angle)*radius,146+std::sin(angle)*radius,2);}
 mixerText(g,source,{422,7,164,26},13,purple);g.setColour(purple);g.drawRoundedRectangle({551,39,23,32},3,1);for(int i=0;i<5;++i)g.fillRect(554.f,65-i*4.f,16.f*(std::clamp(input,0.f,1.f)),2.f);
}
inline void drawCaptureScreen(juce::Graphics& g,juce::Rectangle<float> bounds,bool sampling,bool armed,double elapsed,double limit,float peak,float threshold,float gain,bool gated,bool output,juce::String source){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,bounds);const auto blue=mixerColours[0],ochre=mixerColours[1],grey=mixerColours[2],red=mixerColours[3];
 mixerText(g,output?"MAIN OUTPUT":source,{18,9,568,31},19,grey);
 g.setColour(sampling&&!armed?red:blue.withAlpha(.65f));g.fillRoundedRectangle({155,77,294,60},9);mixerText(g,sampling?(armed?"STANDBY":"RECORDING!"):"ARM / RECORD",{155,77,294,60},35,sampling&&!armed?juce::Colours::white:juce::Colour(0xffc6cebb));
 const float width=340;g.setColour(grey);for(int i=0;i<=6;++i){float x=110+i*width/6;g.drawLine(x,178,x,190,1);mixerText(g,juce::String(limit*i/6,limit<=6?0:1),{x-19,193,38,24},17,grey);}mixerText(g,"SEC",{466,177,55,27},19,grey);g.setColour(red);g.drawLine(110,184,110+(float)std::clamp(elapsed/limit,0.,1.)*width,184,3);
 mixerText(g,juce::String(elapsed,2)+" / "+juce::String(limit,0)+"s",{188,224,226,29},23,ochre);mixerText(g,gated?"HOLD KEY TO CAPTURE":"CLICK ARM TO START / STOP",{130,265,344,22},14,grey);
 g.setColour(blue.withAlpha(.3f));g.fillRect(35.f,64.f,15.f,164.f);g.setColour(peak>1?red:ochre);g.fillRect(35.f,228-164*std::clamp(peak,0.f,1.f),15.f,164*std::clamp(peak,0.f,1.f));g.setColour(red);g.drawLine(30,228-threshold*164,55,228-threshold*164,2);mixerText(g,"IN",{19,233,45,23},15,grey);mixerText(g,"GAIN "+juce::String(gain,2),{449,238,140,25},18,red);
}
}
