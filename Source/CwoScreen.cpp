#include "CwoScreen.h"
#include <cmath>
namespace field {
void drawCwoScreen(juce::Graphics& g,juce::Rectangle<float> bounds,const std::array<float,4>& macros,const CwoAnimation& motion){
    juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(bounds.toNearestInt());g.addTransform(juce::AffineTransform::scale(bounds.getWidth()/604.f,bounds.getHeight()/292.f).translated(bounds.getX(),bounds.getY()));
    const juce::Colour blue{0xff486da2},ochre{0xffa78137},grey{0xff78858b},white{0xffb3b7b4},red{0xffff0800},purple{0xff777088},teal{0xff00636c};
    const float frequency=juce::jlimit(0.f,1.f,macros[0]),delay=juce::jlimit(0.f,1.f,macros[1]),feedback=juce::jlimit(0.f,1.f,macros[2]),sideband=juce::jlimit(0.f,1.f,macros[3]);
    g.fillAll(juce::Colours::black);
    auto text=[&](const juce::String& value,juce::Rectangle<float> area,float size,juce::Colour colour,int justification=juce::Justification::centredLeft){g.setColour(colour);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(value,area,justification,true);};
    text("FREQ",{42,22,90,23},18,blue);text("DELAY",{42,153,95,23},18,ochre);text(juce::String((int)std::round(delay*99)),{39,176,105,61},58,white);
    text("FEEDBACK",{460,22,118,23},18,grey);text(juce::String((int)std::round(feedback*99)),{473,47,110,62},60,white);
    text("SIDEBAND",{462,153,123,23},18,red);
    constexpr float cx=70,cy=93;for(int i=0;i<=10;++i){const float a=-1.4f+i*.28f;g.setColour(white);g.drawLine(cx+std::sin(a)*29,cy-std::cos(a)*29,cx+std::sin(a)*37,cy-std::cos(a)*37,2);}
    const float angle=-1.4f+frequency*2.8f;g.setColour(blue);g.fillEllipse(cx-4,cy-4,8,8);g.drawLine(cx,cy,cx+std::sin(angle)*35,cy-std::cos(angle)*35,2.2f);
    for(int step=0;step<13;++step){g.setColour(step<(int)std::round(sideband*13)?red:white.withAlpha(.7f));float y=257-step*7.f;g.drawLine(501,y,519,y,2);}

    // Original cow silhouette drawn as vectors from the supplied layout.
    juce::Path cow;cow.startNewSubPath(169,74);cow.cubicTo(155,70,152,77,148,84);cow.lineTo(141,89);cow.lineTo(132,92);cow.lineTo(130,98);cow.lineTo(135,105);cow.lineTo(134,111);cow.lineTo(139,114);cow.lineTo(149,108);cow.cubicTo(161,111,174,109,179,103);
    cow.cubicTo(190,119,192,135,197,149);cow.cubicTo(201,166,207,171,215,178);cow.lineTo(222,193);cow.lineTo(227,215);cow.lineTo(225,249);cow.lineTo(219,261);cow.lineTo(218,271);cow.lineTo(247,271);cow.lineTo(249,265);cow.lineTo(241,262);cow.lineTo(244,232);cow.lineTo(250,209);cow.lineTo(254,194);
    cow.cubicTo(271,197,285,199,301,197);cow.cubicTo(316,202,327,203,333,208);cow.lineTo(334,216);cow.lineTo(340,217);cow.lineTo(341,209);cow.lineTo(346,210);cow.lineTo(347,221);cow.lineTo(353,220);cow.lineTo(354,211);cow.lineTo(361,209);cow.cubicTo(370,214,378,211,385,204);
    cow.lineTo(385,230);cow.lineTo(380,255);cow.lineTo(372,263);cow.lineTo(369,271);cow.lineTo(398,271);cow.lineTo(399,266);cow.lineTo(392,264);cow.lineTo(400,242);cow.lineTo(403,220);cow.lineTo(407,201);cow.lineTo(411,193);
    cow.lineTo(417,218);cow.lineTo(424,232);cow.lineTo(423,256);cow.lineTo(416,264);cow.lineTo(413,271);cow.lineTo(441,271);cow.lineTo(439,263);cow.lineTo(432,259);cow.lineTo(434,233);cow.lineTo(440,215);cow.lineTo(439,201);cow.lineTo(432,186);cow.lineTo(430,168);
    cow.cubicTo(429,150,430,133,431,115);cow.cubicTo(433,97,432,86,425,81);cow.cubicTo(415,71,399,78,382,79);cow.lineTo(279,79);cow.cubicTo(265,76,249,79,236,76);cow.lineTo(216,69);cow.lineTo(200,72);cow.lineTo(193,67);cow.lineTo(188,61);cow.lineTo(184,55);cow.lineTo(177,54);cow.lineTo(171,60);cow.lineTo(162,63);cow.lineTo(154,60);cow.lineTo(145,60);cow.lineTo(140,64);cow.lineTo(142,69);cow.lineTo(152,72);cow.lineTo(156,64);
    g.setColour(grey.withAlpha(.78f));g.strokePath(cow,juce::PathStrokeType(2.1f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    juce::Path detail;detail.startNewSubPath(192,66);detail.cubicTo(205,59,215,62,209,69);detail.lineTo(204,73);detail.startNewSubPath(203,64);detail.cubicTo(199,68,201,73,205,73);detail.startNewSubPath(155,77);detail.lineTo(147,88);detail.startNewSubPath(151,96);detail.cubicTo(143,93,145,104,148,107);detail.startNewSubPath(185,70);detail.cubicTo(177,70,174,73,174,76);detail.startNewSubPath(429,86);detail.cubicTo(440,111,435,153,431,185);detail.lineTo(442,207);detail.lineTo(445,211);detail.lineTo(447,205);detail.lineTo(441,191);detail.cubicTo(439,165,442,137,438,113);detail.cubicTo(437,96,433,82,425,81);
    detail.startNewSubPath(252,185);detail.lineTo(249,206);detail.lineTo(239,241);detail.lineTo(239,264);detail.startNewSubPath(238,181);detail.lineTo(231,211);detail.lineTo(233,247);detail.lineTo(230,261);detail.startNewSubPath(407,187);detail.lineTo(401,221);detail.lineTo(398,234);detail.startNewSubPath(420,183);detail.cubicTo(414,201,420,218,428,226);
    g.strokePath(detail,juce::PathStrokeType(1.7f));g.setColour(white);g.fillEllipse(173,72,3,3);

    // Anatomy is fixed. The signal mechanisms move with control-dependent
    // phases; their exact hardware timing remains unmeasured.
    constexpr float tau=juce::MathConstants<float>::twoPi;
    for(int wire=0;wire<3;++wire){float y=92+wire*7.f;juce::Path path;path.startNewSubPath(183,y);path.cubicTo(213,y,217,y+29,245,y+29);g.setColour(purple);g.strokePath(path,juce::PathStrokeType(1.7f));
        const float progress=std::fmod(motion.flowTurns+wire/3.f,1.f);const auto point=path.getPointAlongPath(path.getLength()*progress);g.setColour(ochre);g.fillEllipse(point.x-1.7f,point.y-1.7f,3.4f,3.4f);
    }
    g.setColour(purple);g.drawEllipse(270,92,93,91,2);
    g.setColour(juce::Colours::black);g.fillRect(253.f,112.f,28.f,64.f);g.fillRect(349.f,119.f,39.f,32.f);
    // Ochre flow valve: its rate decreases as Delay increases.
    g.setColour(ochre);g.drawEllipse(253,98,35,35,2);const float valve=motion.flowTurns*tau;
    g.drawLine(270-std::cos(valve)*9,115-std::sin(valve)*9,270+std::cos(valve)*9,115+std::sin(valve)*9,2);
    g.setColour(purple);g.drawLine(270,133,270,142,2);g.drawEllipse(253,139,35,35,2);
    const juce::Colour mixerColours[]{ochre,blue,ochre,teal};for(int dot=0;dot<4;++dot){const float at=motion.frequencyTurns*tau+dot*tau/4;g.setColour(mixerColours[dot]);g.fillEllipse(270+std::cos(at)*7-3.5f,157+std::sin(at)*7-3.5f,7,7);}
    // The red oscillator keeps its amplitude; Sideband changes the blend
    // indication in the right-hand chamber, not a fictitious output level.
    g.setColour(red);juce::Path oscillation;const float amplitude=23;
    oscillation.startNewSubPath(359,124);for(int step=0;step<3;++step){float x=359+step*22.f;oscillation.lineTo(x+7,124);oscillation.quadraticTo(x+12,124,x+12,119);oscillation.lineTo(x+12,124-amplitude+4);oscillation.quadraticTo(x+12,124-amplitude,x+17,124-amplitude);oscillation.quadraticTo(x+22,124-amplitude,x+22,128-amplitude);oscillation.lineTo(x+22,120);}g.strokePath(oscillation,juce::PathStrokeType(2));
    {juce::Graphics::ScopedSaveState rotation(g);juce::Path circle;circle.addEllipse(274,96,85,83);g.reduceClipRegion(circle);g.addTransform(juce::AffineTransform::rotation(motion.frequencyTurns*tau,316.5f,137.5f));
        for(int col=0;col<5;++col){g.setColour(col%2?ochre:purple);float x=292+col*13.f;for(int dash=0;dash<5;++dash)g.drawLine(x,111+dash*12.f,x,118+dash*12.f,1.6f);}
    }
    // Grey Feedback increases return-loop activity, rather than changing the
    // cow's silhouette. This visualization is an inferred design choice.
    for(int particle=0;particle<(int)std::round(feedback*8);++particle){const float at=(motion.flowTurns+particle/8.f)*tau;g.setColour(white.withAlpha(.35f+feedback*.6f));g.fillEllipse(316.5f+std::cos(at)*46-1.6f,137.5f+std::sin(at)*45-1.6f,3.2f,3.2f);}
    g.setColour(purple);g.drawLine(359,143,371,148,2);g.drawLine(359,151,371,156,2);g.drawEllipse(371,135,36,36,2);
    for(int line=0;line<5;++line){g.setColour((line%2?red:purple).withAlpha(.2f+sideband*.8f));g.drawLine(377,141+line*5.f,401,141+line*5.f,1.2f);}
    for(int tick=0;tick<29;++tick){float x=30+tick*19.f;float height=4+5*(.5f+.5f*std::sin(tick*1.7f+delay*4));g.setColour(teal);g.drawLine(x,272,x,272-height,1.4f);}
}
}
