#pragma once
#include "MixerScreen.h"
namespace field {
// Original vector rendering; readings come from the running engine.
inline void drawTempoScreen(juce::Graphics& g,juce::Rectangle<float> bounds,float song,float effective,float speed,int mode,bool host,bool locked,bool relay,bool linked,bool click,float level,int sound,float time,bool playing){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,bounds);
 const auto blue=mixerColours[0],grey=mixerColours[2],ochre=mixerColours[1],red=mixerColours[3];const juce::Colour pale(0xffc6cebb),purple(0xff632654);
 const auto link=host||mode==1?(locked||host?red:grey):mode==2?ochre:grey.withAlpha(.5f);
 mixerText(g,"SONG BPM",{22,8,163,30},25,blue);mixerText(g,"TAPE SPEED",{369,8,186,30},25,grey);
 g.setColour(link);g.drawLine(196,27,252,27,2);g.drawLine(308,27,366,27,2);g.fillEllipse(190,21,12,12);g.fillEllipse(360,21,12,12);mixerText(g,"LINK",{254,10,55,30},19,link);
 g.setColour(purple);g.drawRect(22.f,43.f,188.f,230.f,1.f);g.drawRect(367.f,43.f,188.f,230.f,1.f);g.drawLine(22,160,210,160,1);g.drawLine(367,160,555,160,1);g.drawLine(367,217,555,217,1);g.drawLine(462,160,462,273,1);
 mixerText(g,juce::String(song,std::abs(song-std::round(song))<.001f?0:2),{25,53,182,99},69,blue);mixerText(g,"PLAY BPM",{29,167,174,29},25,grey);
 const float playBpm=song*std::abs(speed);
 mixerText(g,juce::String(playBpm,1),{28,200,177,64},47,pale.withAlpha(.8f));
 mixerText(g,juce::String(speed,2)+":1",{369,56,184,96},58,pale.withAlpha(.8f));mixerText(g,juce::String(std::abs(speed)*19.05f,1),{371,166,88,45},29,pale);mixerText(g,"CMS",{465,166,87,45},26,grey);mixerText(g,juce::String(std::abs(speed)*7.5f,2),{371,222,88,43},29,pale);mixerText(g,"IPS",{465,222,87,43},26,grey);
 const juce::String name=host?"DAW SYNC":mode==0?"FREE":mode==2?"BEAT\nMATCH":relay?"MIDI IN\n+ RELAY":"MIDI\nSYNC";
 g.setColour(ochre.withAlpha(.8f));g.drawRoundedRectangle({217,213,143,60},9,2);g.setColour(pale);g.setFont(juce::Font(juce::FontOptions(25.f)));g.drawFittedText(name,{220,216,137,55},juce::Justification::centred,2);
 if(mode==0&&!host){g.setColour(grey);juce::Path frame;frame.addTriangle(263,172,285,72,307,172);g.strokePath(frame,juce::PathStrokeType(2));float angle=click&&playing?std::sin(time*effective/60*6.283185f)*.35f:0;g.setColour(red);g.drawLine(285,163,285+std::sin(angle)*70,163-std::cos(angle)*70,2);g.setColour(pale);g.fillEllipse(281+std::sin(angle)*51,159-std::cos(angle)*51,8,8);}
 else {mixerText(g,host?"HOST CLOCK":mode==1?(locked?"EXT CLOCK":"WAIT CLOCK"):"MIDI OUT",{213,60,149,26},17,link);g.setColour(link);g.drawEllipse(266,111,42,42,2);for(int i=0;i<5;++i){float a=3.14f+i*.7854f;g.fillEllipse(285+std::cos(a)*13,133+std::sin(a)*13,4,4);}if(relay&&mode==1)mixerText(g,"THRU",{251,165,80,22},16,ochre);}
 mixerText(g,linked?(mode==0&&!host?"LINK ARMED":"TAPE LINK"):"UNLINKED",{216,184,145,23},14,linked?ochre:grey);
 g.setColour(click?red:grey);juce::Path icon;icon.addTriangle(572,37,579,17,586,37);g.strokePath(icon,juce::PathStrokeType(1.5f));g.drawLine(579,22,579,36,1);g.drawLine(579,45,579,270,1);g.fillEllipse(574,263-level*210,10,10);mixerText(g,juce::StringArray{"CLICK","WOOD","BEEP"}[std::clamp(sound,0,2)],{218,278,141,14},11,click?red:grey);
}
}
