#pragma once
#include "MixerScreen.h"
namespace field {
inline void drawStringScreen(juce::Graphics& g,juce::Rectangle<float> b,const std::array<float,4>& v){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,b);const juce::Colour blue(0xff416599),ochre(0xffa78032),pale(0xffc6cebb),purple(0xff54213e);
 mixerText(g,"+",{48,20,35,30},30,pale);mixerText(g,"-",{48,205,35,30},30,pale);mixerText(g,"IMP",{24,102,62,36},27,ochre);mixerText(g,"TENSION",{12,233,128,34},26,blue);mixerText(g,"IMP TYPE",{420,25,155,34},27,juce::Colours::red);
 for(int i=0;i<22;++i){float y=37+i*8.f;g.setColour(i<7?purple:ochre.withAlpha(.7f));g.drawLine(108-(i>6?i*.7f:0),y,128,y,2);}
 g.setColour(purple);g.drawLine(571,36,571,145,2);g.setColour(juce::Colours::red);g.drawLine(571,145,571,145-v[3]*109,2);
 auto point=[](float x,float y){return juce::Point<float>(164+x*183+y*175,196+x*55-y*39);};g.setColour(blue.withAlpha(.4f));for(int i=0;i<=12;++i){float t=i/12.f;auto a=point(t,0),z=point(t,1);g.drawLine(a.x,a.y,z.x,z.y,1);a=point(0,t);z=point(1,t);g.drawLine(a.x,a.y,z.x,z.y,1);}
 for(int line=0;line<16;++line){float t=line/15.f;auto colour=t<.33f?blue:t<.6f?ochre:t<.8f?pale:juce::Colours::red;juce::Path path;float startX=149+t*218;path.startNewSubPath(startX,37);for(int k=1;k<=48;++k){float u=k/48.f,fall=juce::jlimit(0.f,1.f,(u-.25f)/.75f);float wobble=std::sin(u*(10+v[0]*12)+t*v[2]*6)*(1-u)*(3+v[3]*12);float x=startX+u*(85+v[0]*22)+wobble,y=37+fall*(145+t*20)+u*35+v[1]*u*13;path.lineTo(x,y);}g.setColour(colour.withAlpha(.8f));g.strokePath(path,juce::PathStrokeType(1.7f));}
 g.setColour(pale);g.fillEllipse(402+v[2]*18,232,11,11);g.fillEllipse(450-v[2]*18,220,11,11);
}
inline void drawVoltageScreen(juce::Graphics& g,juce::Rectangle<float> b,const std::array<float,4>& v,float peak){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,b);const juce::Colour blue(0xff416599),grey(0xff62717a),pale(0xffc6cebb),brown(0xff4a4436);
 g.setColour(pale);g.drawLine(126,59,476,59,2);juce::Path wave;for(int i=0;i<180;++i){float u=i/179.f,env=std::exp(-std::pow((u-.5f)*6,2));float x=126+350*u,y=59+env*(std::sin(u*(70+v[0]*100))*12+std::sin(u*231)*v[1]*11);if(!i)wave.startNewSubPath(x,y);else wave.lineTo(x,y);}g.setColour(pale);g.strokePath(wave,juce::PathStrokeType(1.8f));
 g.setColour(juce::Colours::red);g.fillEllipse(111,44,30,30);g.setColour(juce::Colours::black);g.drawLine(116,59,136,59,1.5f);g.drawLine(126,49,126,69,1.5f);g.setColour(grey);g.fillEllipse(461,44,30,30);g.setColour(juce::Colours::black);g.drawLine(465,59,487,59,1.5f);g.setColour(brown);g.drawLine(476,74,476,115,1);g.drawLine(476,115,380-v[1]*70,115,1);mixerText(g,"GROUND NOISE",{225,100,160,30},19,brown);
 auto dial=[&](float x,float value,juce::Colour colour){g.setColour(colour);g.fillEllipse(x-30,172,60,60);float a=-2.4f+value*4.8f;g.drawLine(x,202,x+std::sin(a)*45,202-std::cos(a)*45,16);g.setColour(juce::Colours::black);g.fillEllipse(x-9,193,18,18);g.setColour(pale);auto p=juce::Point<float>(x+std::sin(a)*25,202-std::cos(a)*25);g.fillEllipse(p.x-4,p.y-4,8,8);};
 mixerText(g,"AMPERE",{90,130,125,32},20,grey);mixerText(g,"VOLT",{416,130,110,32},20,grey);dial(147,v[0],blue);dial(467,v[3],juce::Colours::red);
 g.setColour(juce::Colour(0xff1c2930));g.drawRoundedRectangle({245,147,121,123},3,5);mixerText(g,"V",{248,151,30,28},21,pale);for(int i=0;i<12;++i){float a=-1.1f+i*.1f;g.setColour(grey);g.drawLine(344+std::sin(a)*72,249-std::cos(a)*72,344+std::sin(a)*81,249-std::cos(a)*81,1);}g.setColour(blue);float a=-1.1f+v[2]*1.1f;g.drawLine(344,249,344+std::sin(a)*79,249-std::cos(a)*79,1.6f);g.setColour(juce::Colours::red);a=-1.1f+juce::jlimit(0.f,1.f,peak)*1.1f;g.drawLine(344,249,344+std::sin(a)*76,249-std::cos(a)*76,1.6f);g.fillEllipse(340,245,8,8);
}
inline void drawPulseScreen(juce::Graphics& g,juce::Rectangle<float> b,const std::array<float,4>& v){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,b);const juce::Colour blue(0xff416599),grey(0xff89979b),ochre(0xffa78032);
 mixerText(g,"FILTER",{15,10,115,38},27,blue);for(int i=0;i<3;++i){g.setColour(blue.withAlpha(.2f+v[0]*.8f));g.drawEllipse(128+i*15.f,22,17,17,1.5f);}g.setColour(blue);juce::Path top;top.startNewSubPath(205,39);top.lineTo(250,39);top.lineTo(250,21);top.lineTo(274,21);top.lineTo(274,39);top.lineTo(309,39);top.lineTo(309,24);top.lineTo(332,24);top.lineTo(332,39);top.lineTo(393,39);g.strokePath(top,juce::PathStrokeType(1.5f));mixerText(g,"MOD",{412,10,80,38},27,juce::Colours::red);
 g.setColour(juce::Colours::red);juce::Path modulation;for(int i=0;i<16;++i){float x=490+i*6.f,y=30+std::round(std::sin(i*.5f)*v[3]*10/3)*3;if(!i)modulation.startNewSubPath(x,y);else{modulation.lineTo(x,y);}}g.strokePath(modulation,juce::PathStrokeType(1.3f));g.drawLine(490,39,585,39,1);
 float h=35+v[1]*130,second=h*(.15f+v[2]*.8f);g.setColour(grey);g.drawLine(20,252,584,252,1.5f);for(int pair=0;pair<2;++pair){float x=160+pair*235.f;g.setColour(juce::Colour(0xff292723));g.fillRect(x,252-h,24.f,h);g.fillRect(x+47,252-second,24.f,second);g.setColour(grey);g.drawRect(x,252-h,24.f,h,1.5f);g.drawRect(x+47,252-second,24.f,second,1.5f);}g.setColour(ochre);g.drawLine(79,252-h,79,252,1);g.fillEllipse(73,246-h,12,12);g.fillEllipse(73,246,12,12);mixerText(g,"AMPL",{15,146,105,38},27,ochre);mixerText(g,"TIME",{190,89,104,38},27,grey);
}
struct SamplerScreenState {std::array<float,4> markers{{0,0,1,1}};double viewStart=0,viewEnd=1;float tune=0,fade=0,gain=1;bool reverse=false,loop=false,ready=false;};
template<class Preview> inline void drawSamplerScreen(juce::Graphics& g,juce::Rectangle<float> b,const Preview& preview,const SamplerScreenState& s){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,b);const juce::Colour pale(0xffc6cebb),grey(0xff9a9d9c);mixerText(g,juce::String::fromUTF8(s.reverse?"◀":"▶"),{12,8,47,40},28,mixerColours[0]);mixerText(g,"TUNE "+juce::String(s.tune*100,2)+"c",{70,8,235,40},27,mixerColours[1]);mixerText(g,"FADE "+juce::String(s.fade*100,0),{337,8,143,40},27,grey);mixerText(g,s.gain<=0?"MUTE":juce::String(20*std::log10(s.gain),1)+" dB",{475,8,118,40},23,juce::Colours::red);
 if(!s.ready){mixerText(g,"IMPORT OR RECORD AUDIO",{30,120,544,60},28,pale);return;}
 auto xpos=[&](double value){return 20+564*(float)((value-s.viewStart)/juce::jmax(.000001,s.viewEnd-s.viewStart));};
 for(int ch=0;ch<2;++ch){float y=63+ch*106.f;g.setColour(grey);g.fillRoundedRectangle({20,y,564,100},5);{juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(juce::Rectangle<int>(20,(int)y,564,100));g.setColour(pale);g.fillRect(xpos(s.markers[0]),y,juce::jmax(0.f,xpos(s.markers[3])-xpos(s.markers[0])),100.f);if(s.loop&&s.fade>0){g.setColour(grey);float fade=(s.markers[2]-s.markers[1])*.5f*s.fade;juce::Path wedge;wedge.addTriangle(xpos(s.markers[2]-fade),y,xpos(s.markers[2]),y,xpos(s.markers[2]),y+100);g.fillPath(wedge);}
 g.setColour(juce::Colours::black);g.drawLine(20,y+50,584,y+50,1);for(int i=0;i<256;++i){const auto sample=preview[(size_t)ch][(size_t)i];float x=20+i/255.f*564;g.drawLine(x,y+50-juce::jlimit(-1.f,1.f,sample.high)*46,x,y+50-juce::jlimit(-1.f,1.f,sample.low)*46,2.3f);}}
 mixerText(g,ch?"R":"L",{20,y,25,27},20,juce::Colours::black);
 }
 for(int marker=0;marker<4;++marker){float x=xpos(s.markers[(size_t)marker]);if(x<20||x>584)continue;g.setColour(mixerColours[(size_t)marker]);g.drawLine(x,63,x,269,1.5f);g.fillRect(x-11,57.f,22.f,6.f);g.fillRect(x-11,269.f,22.f,6.f);}
}
}
