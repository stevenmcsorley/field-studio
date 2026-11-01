#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <cmath>
namespace field {
inline const std::array<juce::Colour,4> mixerColours{{juce::Colour(0xff416599),juce::Colour(0xffb49151),juce::Colour(0xff89979b),juce::Colour(0xffff0900)}};
inline void mixerCanvas(juce::Graphics& g,juce::Rectangle<float> b){g.reduceClipRegion(b.toNearestInt());g.addTransform(juce::AffineTransform::scale(b.getWidth()/604,b.getHeight()/292).translated(b.getX(),b.getY()));g.fillAll(juce::Colours::black);}
inline void mixerText(juce::Graphics& g,juce::String s,juce::Rectangle<float> r,float size,juce::Colour c){g.setColour(c);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(s,r,juce::Justification::centred,true);}
inline void drawMixerScreen(juce::Graphics& g,juce::Rectangle<float> b,const std::array<float,4>& levels,const std::array<float,4>& pans,const std::array<float,4>& peaks,const std::array<bool,4>& muted,const std::array<bool,4>& solo){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,b);const juce::Colour grey(0xff53636c);
 for(int ch=0;ch<4;++ch){float x=ch*141.f+88;auto colour=mixerColours[(size_t)ch];if(ch){g.setColour(juce::Colour(0xff122b37));g.drawLine(x-70,16,x-70,279,1);}
  auto pt=[&](float radius,float angle){return juce::Point<float>(x+std::sin(angle)*radius,112-std::cos(angle)*radius);};
  for(int tick=0;tick<=20;++tick){float angle=-1.05f+tick*.105f;auto a=pt(69,angle),z=pt(tick%5==0?83.f:77.f,angle);g.setColour(tick>=15?juce::Colours::red:grey);g.drawLine(a.x,a.y,z.x,z.y,tick>=15?3.f:1.f);}
  for(int i=0;i<5;++i){auto p=pt(93,-1.05f+i*.525f);mixerText(g,juce::StringArray{"-20","-10","-5","0","+3"}[i],{p.x-16,p.y-7,32,14},10,i==4?juce::Colours::red:grey);}
  float db=20*std::log10(juce::jmax(.00001f,peaks[(size_t)ch]));float angle=-1.05f+juce::jlimit(0.f,1.f,(db+20)/23)*2.1f;auto needle=pt(80,angle);g.setColour(muted[(size_t)ch]?grey:juce::Colours::red);g.drawLine(x,80,needle.x,needle.y,1.5f);
  mixerText(g,juce::String(ch+1),{x-25,45,50,40},35,grey.withAlpha(.6f));
  g.setColour(colour.withAlpha(.45f));juce::Path arc;arc.addCentredArc(x,142,39,58,0,-.55f,.55f,true);g.strokePath(arc,juce::PathStrokeType(1.4f));
  g.setColour(muted[(size_t)ch]?colour.withAlpha(.25f):colour);g.fillEllipse(x-18,124,36,36);float pan=juce::jlimit(-1.f,1.f,pans[(size_t)ch]);g.setColour(juce::Colour(0xffc6cebb));g.drawLine(x,142,x+std::sin(pan*1.25f)*16,142-std::cos(pan*1.25f)*16,2);g.fillEllipse(x-4,138,8,8);
  mixerText(g,"L",{x-58,153,24,32},29,grey);mixerText(g,"R",{x+35,153,24,32},29,grey);
  mixerText(g,muted[(size_t)ch]?"MUTE":juce::String((int)std::round(juce::jlimit(0.f,2.f,levels[(size_t)ch])*50)).paddedLeft('0',2),{x-66,193,132,87},muted[(size_t)ch]?30.f:78.f,colour);
  if(solo[(size_t)ch])mixerText(g,"SOLO",{x-40,180,80,15},12,colour);
 }
}
inline void drawEqScreen(juce::Graphics& g,juce::Rectangle<float> b,const std::array<float,3>& gains,const std::array<bool,3>& enabled,float amount,bool active){
 juce::Graphics::ScopedSaveState save(g);mixerCanvas(g,b);const juce::Colour grey(0xff53636c),pale(0xffc6cebb);std::array<float,3> ys{};
 for(int i=0;i<3;++i){float x=90+i*141.f;ys[(size_t)i]=110-gains[(size_t)i]*4;g.setColour(juce::Colour(0xff122b37));g.drawLine(x+71,16,x+71,276,1);mixerText(g,juce::StringArray{"100 HZ","1 KHZ","10 KHZ"}[i],{x-59,28,118,28},20,grey);}
 juce::Path curve;curve.startNewSubPath(20,110-(enabled[0]&&active?gains[0]*amount*4:0));for(int i=0;i<300;++i){float u=i/299.f*2;int a=juce::jmin(1,(int)u);float t=u-a;t=t*t*(3-2*t);float ga=enabled[(size_t)a]&&active?gains[(size_t)a]*amount:0,gb=enabled[(size_t)a+1]&&active?gains[(size_t)a+1]*amount:0;curve.lineTo(90+u*141,110-(ga+(gb-ga)*t)*4);}curve.lineTo(443,110-(enabled[2]&&active?gains[2]*amount*4:0));g.setColour(juce::Colour(0xff71305e));g.strokePath(curve,juce::PathStrokeType(1.5f));
 for(int i=0;i<4;++i){float x=90+i*141.f;for(int tick=0;tick<18;++tick){g.setColour(grey.withAlpha(.65f));g.drawLine(x-10,80+tick*8.f,x+10,80+tick*8.f,2);}float y=i<3?ys[(size_t)i]:224-amount*144;auto c=mixerColours[(size_t)i];g.setColour((i<3?!enabled[(size_t)i]:!active)?c.withAlpha(.25f):c);g.fillRect(x-19,y-9,38.f,18.f);g.setColour(pale);g.drawLine(x-19,y,x+19,y,1);if(i<3)mixerText(g,juce::StringArray{"LOW","MID","HIGH"}[i],{x-58,244,116,32},27,pale);}
 g.setColour(active?juce::Colours::red:grey);g.fillRoundedRectangle({450,18,132,30},5);mixerText(g,"MAX EQ",{450,18,132,30},25,juce::Colours::black);g.setColour(pale);g.fillRoundedRectangle({450,244,132,30},5);mixerText(g,"CLEAN",{450,244,132,30},25,juce::Colours::black);
}
}
