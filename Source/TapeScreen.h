#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "LfoGlyphs.h"
#include <array>
#include <vector>
#include <cmath>
#include <limits>
namespace field {
struct TapeScreenState {
    double position=0,visible=16,viewStart=std::numeric_limits<double>::quiet_NaN(),loopIn=0,loopOut=4,countIn=0;
    double travel=std::numeric_limits<double>::quiet_NaN(),writeBegin=0,writeEnd=0;
    float speed=1,transportSpeed=0,recordLevel=1,recordPan=0,tempo=110;
    int track=0,style=0;
    bool playing=false,recording=false,shifted=false,loopEnabled=false;
    std::array<double,4> durations{};
    std::array<std::vector<double>,4> cuts;
};
// Shared geometry for painting and pointer seeking, including a scrolled view.
inline double tapeScreenTime(float x,float left,float width,double visible,double origin=0){return juce::jlimit(0.,360.,origin+(double)((x-left-width*20.f/604.f)/(width*564.f/604.f))*visible);}
inline void drawTapeScreen(juce::Graphics& g,juce::Rectangle<float> bounds,const TapeScreenState& s){
    juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(bounds.toNearestInt());g.addTransform(juce::AffineTransform::scale(bounds.getWidth()/604.f,bounds.getHeight()/292.f).translated(bounds.getX(),bounds.getY()));g.fillAll(juce::Colours::black);
    const juce::Colour pale{0xffc6cebb},grey{0xff62717a},ochre{0xffa78040},blue{0xff416599},red{0xffff0600},brown{0xff30281f};
    const auto ink=s.recording&&s.countIn<=0?red:pale;
    auto text=[&](juce::String v,juce::Rectangle<float> box,float size,juce::Colour c,int align=juce::Justification::centred){g.setColour(c);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(v,box,align,true);};
    const int ticks=(int)std::floor(juce::jlimit(0.,360.,s.position)*100+.0001);
    const int groups[]{ticks/6000,ticks/100%60,ticks%100};
    for(int i=0;i<3;++i){const float x=235+i*49.f;drawLfoText(g,juce::String(groups[i]).paddedLeft('0',2),{x,20,40,25},24,pale,1.6f);if(i<2){g.setColour(pale);g.fillEllipse(x+43,27,2,2);g.fillEllipse(x+43,37,2,2);}}
    drawLfoText(g,juce::String(s.track+1),{14,18,32,39},38,pale,1.5f);
    const double travel=std::isfinite(s.travel)?s.travel:s.position;
    const float phase=(float)std::remainder(travel*1.3,6.283185307179586);
    auto point=[](float x,float y,float radius,float angle){return juce::Point<float>(x+std::sin(angle)*radius,y-std::cos(angle)*radius);};
    auto hub=[&](float x,float y,float radius,float angle,juce::Colour colour){
        g.setColour(colour);g.fillEllipse(x-radius,y-radius,radius*2,radius*2);g.setColour(juce::Colours::black);g.fillEllipse(x-radius*.55f,y-radius*.55f,radius*1.1f,radius*1.1f);g.setColour(colour);g.fillEllipse(x-3,y-3,6,6);
        for(int spoke=0;spoke<3;++spoke){auto p=point(x,y,radius*.68f,angle+spoke*2.0943951f);g.setColour(juce::Colours::black);g.drawLine(x,y,p.x,p.y,juce::jmax(2.f,radius*.11f));}
    };
    if(s.style==0||s.style==1){
        for(int reel=0;reel<2;++reel){const float x=reel==0?181.f:423.f,y=109;
            const float amount=(float)juce::jlimit(0.,1.,s.position/360.);const float pack=39+14*std::sqrt(reel==0?1-amount:amount);
            g.setColour(brown);g.fillEllipse(x-pack,y-pack,pack*2,pack*2);g.setColour(pale);g.drawEllipse(x-68,y-68,136,136,2);
            if(s.style==1){for(int arm=0;arm<3;++arm){const float angle=phase+arm*2.0943951f;auto a=point(x,y,28,angle),b=point(x,y,61,angle);g.setColour(grey);g.drawLine(a.x,a.y,b.x,b.y,12);}}
            hub(x,y,38,phase,pale);auto p=point(x,y,58,phase);g.setColour(pale.withAlpha(.65f));g.drawLine(x,y,p.x,p.y,1.5f);
        }
        juce::Path path;path.startNewSubPath(162,145);path.lineTo(179,201);path.lineTo(222,192);path.lineTo(302,201);path.lineTo(382,192);path.lineTo(425,201);path.lineTo(443,145);g.setColour(brown.brighter(.35f));g.strokePath(path,juce::PathStrokeType(2));
        for(float x:{222.f,382.f}){g.setColour(grey);g.drawEllipse(x-9,183,18,18,2);}g.setColour(grey);g.fillEllipse(173,195,12,12);g.fillEllipse(419,195,12,12);g.setColour(pale);g.fillRect(290.f,182.f,24.f,20.f);
    }else if(s.style==2){
        // Original vector cassette, based on the public field demonstration.
        g.setColour(ink);g.drawRoundedRectangle({224,61,156,147},4,2);g.drawRect(232.f,69.f,140.f,120.f,1.5f);g.fillRect(239.f,77.f,126.f,25.f);
        g.setColour(juce::Colours::black);g.fillRoundedRectangle({241,118,122,38},16);hub(262,137,17,phase,ink);hub(342,137,17,phase,ink);g.setColour(ink);g.drawLine(279,120,325,120,2);g.drawLine(279,154,325,154,2);
        juce::Path lower;lower.startNewSubPath(245,190);lower.lineTo(255,168);lower.lineTo(349,168);lower.lineTo(359,190);g.strokePath(lower,juce::PathStrokeType(2));
        for(float x:{241.f,363.f})for(float y:{72.f,199.f})g.fillEllipse(x-2,y-2,4,4);
    }else{
        // Disc Mini shell and rotating disc; exact hardware frames unmeasured.
        g.setColour(grey);g.drawRoundedRectangle({222,63,160,145},5,2);g.setColour(ink);g.drawRect(231.f,72.f,139.f,125.f,1.f);g.fillRect(231.f,72.f,53.f,68.f);g.setColour(juce::Colours::black);g.fillRect(242.f,80.f,30.f,46.f);
        g.setColour(ink);g.drawEllipse(266,90,95,95,2);g.drawEllipse(294,118,39,39,2);auto p=point(313.5f,137.5f,41,phase*2);g.drawLine(313.5f,137.5f,p.x,p.y,2);g.fillEllipse(310,134,7,7);
    }
    const float control=s.shifted?(s.recordPan+1)*.5f:juce::jlimit(0.f,1.f,s.recordLevel*.5f);g.setColour(grey);g.drawLine(573,29,573,211,1);g.setColour(red);g.drawLine(573,211,573,211-control*182,2);g.fillEllipse(566,204-control*182,14,14);
    const float cx=s.style>=2?457.f:302.f,cy=s.style>=2?115.f:110.f;
    if(s.countIn>0)text("COUNT "+juce::String((int)std::ceil(s.countIn)),{cx-49,cy-36,98,31},18,ochre);
    else if(s.recording){g.setColour(red);g.fillEllipse(cx-10,cy-10,20,20);}
    else if(s.playing&&s.speed!=0){g.setColour(pale);juce::Path arrow;if(s.speed<0){arrow.addTriangle(cx-5,cy-11,cx-5,cy+11,cx-24,cy);arrow.addTriangle(cx+19,cy-11,cx+19,cy+11,cx,cy);}else arrow.addTriangle(cx-11,cy-11,cx-11,cy+11,cx+10,cy);g.fillPath(arrow);}
    else {g.setColour(pale);g.fillRect(cx-9,cy-9,18.f,18.f);}
    text(juce::String(std::abs(s.speed)*100,0)+"%",{cx-54,cy+18,109,36},s.style>=2?23:29,grey);
    g.setColour(grey);juce::Path source;for(int i=0;i<60;++i){float x=32+i*.8f,y=184+11*std::sin(i*.29f);if(!i)source.startNewSubPath(x,y);else source.lineTo(x,y);}g.strokePath(source,juce::PathStrokeType(2));
    const double visible=juce::jmax(.1,s.visible),origin=std::isfinite(s.viewStart)?s.viewStart:s.position-visible*.5;
    auto tx=[&](double time){return 20+564*(float)((time-origin)/visible);};
    {juce::Graphics::ScopedSaveState lanes(g);g.reduceClipRegion({20,214,564,65});
        const double beat=60./juce::jlimit(30.f,300.f,s.tempo),step=beat*4;
        for(double t=std::ceil(origin/step)*step;t<=origin+visible;t+=step)if(t>=0&&t<=360){g.setColour(grey);const float x=tx(t);g.drawLine(x,218,x,230,1);}
        g.setColour(s.loopEnabled?ochre:ochre.withAlpha(.4f));g.drawLine(tx(s.loopIn),224,tx(s.loopOut),224,2);g.drawEllipse(tx(s.loopIn)-6,218,12,12,2);g.fillEllipse(tx(s.loopOut)-6,218,12,12);
        for(int i=0;i<4;++i){const float y=236+i*9.f;const double duration=s.durations[(size_t)i];g.setColour(brown);g.fillRect(20.f,y,564.f,6.f);
            if(duration>0){g.setColour(i==s.track?ochre:grey);g.fillRect(juce::Rectangle<float>(tx(0),y,tx(duration)-tx(0),6));
                if(i==s.track&&s.position>=0&&s.position<=duration){double start=0,end=duration;for(double cut:s.cuts[(size_t)i]){if(cut<=s.position)start=std::max(start,cut);else end=std::min(end,cut);}g.setColour(blue);g.fillRect(juce::Rectangle<float>(tx(start),y,juce::jmax(0.f,tx(end)-tx(start)),6));}}
            if(i==s.track&&s.recording&&s.countIn<=0&&s.writeEnd>s.writeBegin){g.setColour(red);g.fillRect(juce::Rectangle<float>(tx(s.writeBegin),y,juce::jmax(1.f,tx(s.writeEnd)-tx(s.writeBegin)),6));}
            for(double cut:s.cuts[(size_t)i])if(cut>=origin&&cut<=origin+visible){g.setColour(juce::Colours::black);g.drawLine(tx(cut),y,tx(cut),y+6,1.5f);}
        }
    }
    const float head=tx(s.position);g.setColour(pale);g.drawLine(head,192,head,278,2);
}
}
