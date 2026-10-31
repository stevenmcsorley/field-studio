#include "DelayScreen.h"
#include "DelayControls.h"
namespace field {
void drawDelayScreen(juce::Graphics& g,juce::Rectangle<float> bounds,const std::array<float,4>& value,float phase){
    juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(bounds.toNearestInt());g.addTransform(juce::AffineTransform::scale(bounds.getWidth()/604.f,bounds.getHeight()/292.f).translated(bounds.getX(),bounds.getY()));g.fillAll(juce::Colours::black);
    const juce::Colour blue{0xff416599},ochre{0xffa78032},grey{0xff7c8c93},white{0xffb2b7b6},red{0xffff0600};
    const float range=juce::jlimit(0.f,1.f,value[0]),speed=juce::jlimit(0.f,1.f,value[1]),feedback=juce::jlimit(0.f,1.f,value[2]),level=juce::jlimit(0.f,1.f,value[3]);
    auto text=[&](const juce::String& s,juce::Rectangle<float> area,float size,juce::Colour c){g.setColour(c);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(s,area,juce::Justification::centredLeft,true);};
    text("RANGE",{38,23,130,23},18,blue);const float ms=delayRangeMs(range);text(juce::String(ms,ms==62.5f?1:0)+" MS",{38,42,130,23},18,blue);
    text("SPEED",{38,155,130,23},18,ochre);text(juce::String((int)std::round(speed*99)),{28,175,132,67},61,white);
    text("FEEDBACK",{471,23,126,23},18,grey);text(juce::String((int)std::round(feedback*99)),{489,45,110,63},60,white);
    text("LEVEL",{508,155,90,23},18,red);for(int bar=0;bar<13;++bar){g.setColour(bar<(int)std::round(level*13)?red:white.withAlpha(.8f));g.drawLine(533,258-bar*7.f,550,258-bar*7.f,2);}
    g.setColour(juce::Colour(0xff0e1d28));g.fillRoundedRectangle({185,22,250,248},6);
    {juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(juce::Rectangle<int>(185,22,250,248));
        g.setColour(juce::Colours::black);g.drawEllipse(189,27,241,241,2);
        const float movement=std::sin(phase*(.15f+speed*.6f))*5.f;g.setColour(blue);g.drawEllipse(227+movement,61,194,196,2.3f);
        g.setColour(juce::Colours::black);g.drawEllipse(268,97,158,158,2);g.drawEllipse(306,135,83,83,2);g.drawEllipse(331,160,33,33,2);
        g.setColour(red);g.fillEllipse(341,170,13,13);
    }
}
}
