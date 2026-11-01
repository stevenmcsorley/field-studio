#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>
#include <cmath>
namespace field {
struct TapeScreenState {
    double position=0,visible=16,loopIn=0,loopOut=4,countIn=0;
    float speed=1,recordLevel=1,recordPan=0;
    int track=0;
    bool playing=false,recording=false,shifted=false;
    std::array<double,4> durations{};
    std::array<std::vector<double>,4> cuts;
};
// Shared geometry for painting and pointer seeking. Coordinates are local LCD.
inline double tapeScreenTime(float x,float left,float width,double visible){return juce::jlimit(0.,visible,(double)((x-left-width*20.f/604.f)/(width*564.f/604.f))*visible);}
inline void drawTapeScreen(juce::Graphics& g,juce::Rectangle<float> bounds,const TapeScreenState& s){
    juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(bounds.toNearestInt());g.addTransform(juce::AffineTransform::scale(bounds.getWidth()/604.f,bounds.getHeight()/292.f).translated(bounds.getX(),bounds.getY()));g.fillAll(juce::Colours::black);
    const juce::Colour pale{0xffc6cebb},grey{0xff62717a},ochre{0xffa78040},blue{0xff416599},red{0xffff0600},brown{0xff30281f};
    auto text=[&](juce::String v,juce::Rectangle<float> box,float size,juce::Colour c,int align=juce::Justification::centred){g.setColour(c);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(v,box,align,true);};
    const int ticks=(int)std::floor(juce::jlimit(0.,360.,s.position)*100+.0001);
    text(juce::String(ticks/6000).paddedLeft('0',2)+":"+juce::String(ticks/100%60).paddedLeft('0',2)+":"+juce::String(ticks%100).paddedLeft('0',2),{207,14,190,35},29,pale);
    text(juce::String(s.track+1),{12,13,35,44},39,pale,juce::Justification::centredLeft);
    auto point=[](float x,float y,float radius,float angle){return juce::Point<float>(x+std::sin(angle)*radius,y-std::cos(angle)*radius);};
    const float phase=(float)std::fmod(s.position*1.3,6.283185307179586);
    for(int reel=0;reel<2;++reel){const float x=reel==0?181.f:423.f,y=109;const float amount=(float)juce::jlimit(0.,1.,s.position/360.);float pack=39+14*std::sqrt(reel==0?1-amount:amount);
        g.setColour(brown);g.fillEllipse(x-pack,y-pack,pack*2,pack*2);g.setColour(pale);g.drawEllipse(x-68,y-68,136,136,2);g.fillEllipse(x-38,y-38,76,76);g.setColour(juce::Colours::black);g.fillEllipse(x-21,y-21,42,42);g.setColour(pale);g.fillEllipse(x-4,y-4,8,8);
        for(int spoke=0;spoke<3;++spoke){auto p=point(x,y,25,phase+spoke*2.0943951f);g.setColour(juce::Colours::black);g.drawLine(x,y,p.x,p.y,4);}
        auto p=point(x,y,58,phase);g.setColour(pale.withAlpha(.65f));g.drawLine(x,y,p.x,p.y,1.5f);
    }
    // Tape path, guide rollers and head remain fixed while reel hubs turn.
    juce::Path path;path.startNewSubPath(162,145);path.lineTo(179,201);path.lineTo(222,192);path.lineTo(302,201);path.lineTo(382,192);path.lineTo(425,201);path.lineTo(443,145);g.setColour(brown.brighter(.35f));g.strokePath(path,juce::PathStrokeType(2));
    for(float x:{222.f,382.f}){g.setColour(grey);g.drawEllipse(x-9,183,18,18,2);}g.setColour(grey);g.fillEllipse(173,195,12,12);g.fillEllipse(419,195,12,12);g.setColour(pale);g.fillRect(290.f,182.f,24.f,20.f);
    if(s.countIn>0)text("COUNT "+juce::String((int)std::ceil(s.countIn)),{252,72,100,38},19,ochre);
    else if(s.recording){g.setColour(red);g.fillEllipse(289,94,26,26);}
    else if(s.playing&&s.speed!=0){g.setColour(pale);juce::Path arrow;if(s.speed<0){arrow.addTriangle(297,99,297,121,278,110);arrow.addTriangle(321,99,321,121,302,110);}else arrow.addTriangle(291,99,291,121,312,110);g.fillPath(arrow);}
    else {g.setColour(pale);g.fillRect(293.f,101.f,18.f,18.f);}
    text(juce::String(std::abs(s.speed)*100,0)+"%",{248,128,109,36},29,grey);
    g.setColour(grey);juce::Path source;for(int i=0;i<60;++i){float x=32+i*.8f,y=184+11*std::sin(i*.29f);if(!i)source.startNewSubPath(x,y);else source.lineTo(x,y);}g.strokePath(source,juce::PathStrokeType(2));
    const float control=s.shifted?(s.recordPan+1)*.5f:juce::jlimit(0.f,1.f,s.recordLevel*.5f);g.setColour(grey);g.drawLine(573,29,573,211,1);g.setColour(red);g.drawLine(573,211,573,211-control*182,2);g.fillEllipse(566,204-control*182,14,14);
    const double visible=juce::jmax(.1,s.visible);auto tx=[&](double time){return 20+564*(float)juce::jlimit(0.,1.,time/visible);};
    for(int i=0;i<=12;++i){g.setColour(grey);float x=20+i*47.f;g.drawLine(x,218,x,230,1);}
    g.setColour(ochre);g.drawLine(tx(s.loopIn),224,tx(s.loopOut),224,2);g.drawEllipse(tx(s.loopIn)-6,218,12,12,2);g.fillEllipse(tx(s.loopOut)-6,218,12,12);
    for(int i=0;i<4;++i){float y=236+i*9.f;g.setColour(brown);g.fillRect(20.f,y,564.f,6.f);if(s.durations[(size_t)i]>0){g.setColour(s.recording&&i==s.track?red:i==s.track?blue:grey);g.fillRect(20.f,y,tx(s.durations[(size_t)i])-20,6.f);}for(double cut:s.cuts[(size_t)i])if(cut>=0&&cut<=visible){g.setColour(juce::Colours::black);g.drawLine(tx(cut),y,tx(cut),y+6,1.5f);}}
    const float head=tx(s.position);g.setColour(pale);g.drawLine(head,192,head,278,2);
}
}
