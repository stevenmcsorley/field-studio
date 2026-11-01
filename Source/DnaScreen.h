#pragma once
#include "LcdReadouts.h"
namespace field {
// Parameter illustration. CPU ID is the reference's screen label, not a CPU serial number.
inline void drawDnaScreen(juce::Graphics& g,juce::Rectangle<float> b,const std::array<float,4>& v){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,b);const juce::Colour blue(0xff2d3d64),ochre(0xff8f6836),grey(0xffa1a3a5),link(0xff293139),pale(0xffc4c8b6),red(0xffff0000),purple(0xff542043);
 mixerText(g,"CPU ID",{247,38,102,29},20,juce::Colour(0xff4f4f4d));mixerText(g,"FILTER",{24,133,101,28},20,juce::Colour(0xff3b5787));
 constexpr int colourIndex[]{0,2,1,0,0,0,3,1,3,0,0,2,1};const juce::Colour palette[]{blue,ochre,grey,red};constexpr int lowerColour[]{1,1,2,0,3,1,3,2,2,0,1,1,2};constexpr char bottom[]{'4','D','5','E','E','3','5',' ','D','3','3','E','5'};
 const int variant=(int)std::round(v[1]*31)-15+(int)std::round(v[2]*15)-7;constexpr char hex[]="0123456789ABCDEF";
 for(int i=0;i<13;++i){const float x=143+i*23.5f,top=i==7?96.f:i==11?100.f:112.f,low=i==4||i>=11?199.f:166.f;g.setColour(link);g.drawLine(x+8.5f,top+20,x+8.5f,i==7?148:low,1.5f);
 auto tile=[&](float y,char c,juce::Colour colour){g.setColour(colour);g.fillRect(x,y,17.f,20.f);mixerText(g,juce::String::charToString((juce::juce_wchar)c),{x-1,y-3,19,25},23,colour==grey&&y<130?juce::Colours::black:pale);};
 tile(top,'4',palette[colourIndex[i]]);if(i==7){g.setColour(link);g.fillEllipse(x+3,142,12,12);}else {int code=juce::String(hex).indexOfChar(bottom[i]);char c=variant==0?bottom[i]:hex[(code+variant+i*std::abs(variant)+256)%16];tile(low+(v[2]-.48f)*(i%3-1)*11,c,palette[lowerColour[i]]);}
 }
 // Filter and modifier move terminal nodes; noise controls the red terminal segment.
 g.setColour(ochre);g.fillEllipse(474,101+(v[1]-.48f)*35,12,12);g.setColour(grey);g.fillEllipse(474,186+(v[2]-.48f)*35,12,12);g.setColour(purple);g.drawLine(542,104,542,199,2);g.setColour(red);g.drawLine(542,199-std::min(95.f,v[3]*118),542,199,2);
 g.setColour(blue.withAlpha(.4f));g.drawLine(141,141,141+(v[0]-.48f)*22,141,1);
}
}
