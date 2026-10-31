#include "DSynthScreen.h"
#include <cmath>

namespace field {
void drawDSynthScreen(juce::Graphics& g,juce::Rectangle<float> bounds,const std::array<float,4>& macros,bool shifted,float phase){
    juce::Graphics::ScopedSaveState state(g);g.reduceClipRegion(bounds.toNearestInt());
    g.addTransform(juce::AffineTransform::scale(bounds.getWidth()/604.f,bounds.getHeight()/292.f).translated(bounds.getX(),bounds.getY()));
    const juce::Colour blue{0xff425f91},ochre{0xffa9823e},cream{0xffc8d0bb},grey{0xff60717b},panel{0xff1e292f},red{0xffff0800},dark{0xff293137};
    const float mix=juce::jlimit(0.f,1.f,macros[0]),wave=juce::jlimit(0.f,1.f,macros[1]),env=juce::jlimit(0.f,1.f,macros[2]),cross=juce::jlimit(0.f,1.f,macros[3]);
    g.fillAll(juce::Colours::black);
    auto text=[&](const juce::String& value,juce::Rectangle<float> area,float size,juce::Colour colour,int justification=juce::Justification::centred){g.setColour(colour);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(value,area,justification,true);};
    auto tab=[&](const juce::String& value,juce::Rectangle<float> area,juce::Colour colour){g.setColour(colour);g.fillRoundedRectangle(area,2);text(value,area,18,juce::Colours::black);};
    tab("1",{23,22,17,18},blue);tab("XFADE",{43,22,84,18},blue);tab("OSC 1",{136,22,88,18},ochre);tab("ENV",{233,22,104,18},cream);
    tab("1",{23,125,17,18},blue);text("ENVELOPE",{45,125,61,18},9,cream);tab("2",{109,125,18,18},blue);
    tab("2",{23,151,17,18},shifted?blue:blue.darker(.35f));tab("FREQ",{43,151,84,18},blue);tab("OSC 2",{136,151,88,18},shifted?ochre:dark);tab("ENV",{233,151,104,18},shifted?cream:grey);

    auto knob=[&](float cx,float cy,float value,juce::Colour colour,bool ticks){
        if(ticks){for(int i=0;i<=10;++i){float angle=-2.5f+i*.5f;g.setColour(grey);g.drawLine(cx+std::sin(angle)*27,cy-std::cos(angle)*27,cx+std::sin(angle)*34,cy-std::cos(angle)*34,.9f);}}
        g.setColour(colour);g.fillEllipse(cx-21,cy-21,42,42);const float angle=(value-.5f)*5.f;
        g.setColour(juce::Colours::black);g.drawLine(cx,cy,cx+std::sin(angle)*17,cy-std::cos(angle)*17,2.5f);g.fillEllipse(cx-5,cy-5,10,10);
    };
    knob(75,95,mix,blue,false);text(juce::String(mix*100,0),{56,45,38,18},10,cream);g.setColour(cream);g.fillEllipse(72,63,6,6);
    g.setColour(grey.withAlpha(.5f));g.fillEllipse(46,109,6,6);g.setColour(red);g.fillEllipse(98,109,6,6);
    // The lower ratio dial is a reference-layout illustration. It does not
    // invent a second stored frequency parameter in the four-macro model.
    knob(75,224,.5f,shifted?blue:grey,true);text("1/2",{51,171,48,18},11,cream);text("1/8",{21,251,48,18},11,grey);text("1/1",{91,251,48,18},11,grey);

    auto oscillator=[&](float y,float value,juce::Colour colour){
        g.setColour(colour);g.drawLine(147,y+8,147,y,1);g.drawLine(147,y,162,y+8,1);
        g.drawLine(147,y+45,152,y+45,1);g.drawLine(152,y+45,152,y+38,1);g.drawLine(152,y+38,157,y+38,1);g.drawLine(157,y+38,157,y+45,1);g.drawLine(157,y+45,162,y+45,1);g.drawLine(162,y+45,162,y+38,1);
        juce::Path sine;for(int i=0;i<=24;++i){float x=147+i*15.f/24,yv=y+79-std::sin(i/24.f*juce::MathConstants<float>::twoPi)*4;if(i==0)sine.startNewSubPath(x,yv);else sine.lineTo(x,yv);}g.strokePath(sine,juce::PathStrokeType(1));
        g.setColour(dark);for(int line=0;line<11;++line){float at=y+4+line*7.1f;g.drawLine(176,at,186,at,1);g.drawLine(198,at,208,at,1);}g.drawLine(192,y+2,192,y+81,.7f);
        const float slider=y+6+value*65;g.setColour(colour);g.fillRoundedRectangle({173,slider,39,18},2);g.setColour(cream.withAlpha(.7f));g.drawLine(174,slider+9,211,slider+9,1);
    };
    oscillator(50,wave,ochre);oscillator(180,env,shifted?ochre:grey.withAlpha(.55f));

    auto envelope=[&](float y,float value,juce::Colour colour){
        const int bars=(int)std::round(value*6);for(int i=0;i<7;++i){g.setColour(i<=bars?red:dark);g.fillRect(239.f,y+55-i*8,12.f,6.f);}
        text("LONG DECAY",{252,y-2,84,16},10,cream);knob(287,y+45,value,colour,false);
        g.setColour(grey.withAlpha(.5f));g.fillEllipse(257,y+60,6,6);g.fillEllipse(310,y+60,6,6);text("SHORT",{252,y+76,37,14},10,cream);text("ATTACK",{293,y+76,44,14},10,cream);
    };
    envelope(51,env,cream);envelope(181,env,shifted?cream:grey);

    // Upper-right mode selector, waveform thumbnail and filter marker.
    g.setColour(panel);g.fillRoundedRectangle({344,19,171,58},4);g.fillRoundedRectangle({518,19,63,58},4);
    g.setColour(juce::Colours::black);g.fillRect(391.f,22.f,120.f,51.f);
    g.setColour(juce::Colour(0xff0e3840));for(int i=0;i<7;++i)g.drawLine(392+i*19.f,22,392+i*19.f,72,.6f);for(int i=0;i<4;++i)g.drawLine(392,25+i*14.f,511,25+i*14.f,.6f);
    g.setColour(grey);g.drawLine(450,22,450,72,.8f);
    juce::Path preview;for(int i=0;i<140;++i){float u=i/139.f;float signal=std::sin(u*juce::MathConstants<float>::twoPi*(2+wave*3)+std::sin(phase)*cross*.2f);float x=392+u*119,y=48-signal*11;if(i==0)preview.startNewSubPath(x,y);else preview.lineTo(x,y);}g.setColour(juce::Colour(0xff0d8d96));g.strokePath(preview,juce::PathStrokeType(1));
    for(int i=0;i<2;++i){g.setColour(i==0?red:cream);g.drawEllipse(362,31+i*23.f,9,9,2);}
    g.setColour(cream);g.drawLine(550,26,550,69,2);text("MODE",{344,83,71,23},19,cream);text("FILTER",{512,83,77,23},18,grey);

    // Cross-modulation/filter ring, with the guide's eight numbered positions.
    constexpr float cx=465,cy=192;
    g.setColour(red);g.fillEllipse(cx-50,cy-50,100,100);g.setColour(juce::Colours::black);g.fillEllipse(cx-20,cy-20,40,40);
    for(int tick=0;tick<40;++tick){float angle=juce::MathConstants<float>::twoPi*tick/40;g.setColour(grey);g.drawLine(cx+std::sin(angle)*57,cy-std::cos(angle)*57,cx+std::sin(angle)*73,cy-std::cos(angle)*73,.8f);}
    const int selected=juce::jlimit(0,7,(int)(cross*7.999f));
    const float pointer=(selected-4)*juce::MathConstants<float>::pi/4;
    g.setColour(cream);g.drawLine(cx+std::sin(pointer)*69,cy-std::cos(pointer)*69,cx+std::sin(pointer)*43,cy-std::cos(pointer)*43,1.5f);
    for(int step=0;step<8;++step){float angle=-juce::MathConstants<float>::pi+step*juce::MathConstants<float>::pi/4;float x=cx+std::sin(angle)*91,y=step==0?270.f:cy-std::cos(angle)*91;auto box=juce::Rectangle<float>(22,22).withCentre({x,y});
        if(step==selected){g.setColour(red);g.fillRoundedRectangle(box,3);}else if(step!=0){g.setColour(red);g.drawRoundedRectangle(box,3,1);}
        text(juce::String(step),box,18,step==selected?juce::Colours::black:step==0?cream:red);
    }
}
}
