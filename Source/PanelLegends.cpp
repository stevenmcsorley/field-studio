#include "PluginEditor.h"
#include <cmath>

void FieldLookAndFeel::drawButtonText(juce::Graphics& g,juce::TextButton& button,bool over,bool down){
    if(!button.getProperties().contains("panelKey")&&!button.getProperties().contains("transportKey")&&!button.getProperties().contains("panelUtility")){juce::LookAndFeel_V4::drawButtonText(g,button,over,down);return;}
    juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::scale(button.getWidth()/67.f,button.getHeight()/66.f));
    const int key=(int)button.getProperties()["panelKey"];const auto colour=button.findColour(juce::TextButton::textColourOffId).withMultipliedAlpha(button.isEnabled()?.8f:.35f);g.setColour(colour);
    auto text=[&](const juce::String& s,juce::Rectangle<float> r,float size){g.setColour(colour);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(s,r,juce::Justification::centred,false);};
    if(button.getProperties().contains("transportKey")){
        const int action=(int)button.getProperties()["transportKey"];
        // Recording/count-in states retain their dynamic text.
        if(action==3&&(button.getButtonText()=="ARM"||button.getButtonText()=="SAMPLE")){text(button.getButtonText(),{12,22,43,22},10);return;}
        if(action==0||action==1){float top=action==0?25.f:38.f,bottom=action==0?38.f:25.f;g.drawLine(33.5f,top,33.5f,bottom,1.3f);g.drawLine(28,action==0?30.f:33.f,33.5f,top,1.3f);g.drawLine(39,action==0?30.f:33.f,33.5f,top,1.3f);g.drawLine(24,43,43,43,1.2f);}
        else if(action==2){g.drawEllipse(24,34,8,8,1.2f);g.drawEllipse(35,34,8,8,1.2f);g.drawLine(28,38,41,24,1.3f);g.drawLine(39,38,27,24,1.3f);}
        else if(action==3){g.setColour(juce::Colour(0xffed682b));g.fillEllipse(28,28,11,11);}
        else if(action==4){juce::Path play;play.addTriangle(29,26,29,41,41,33.5f);g.fillPath(play);}
        else if(action==5)g.fillRect(28.f,28.f,11.f,11.f);
        else{const float direction=action==6?-1.f:1.f;g.drawLine(23,33.5f,44,33.5f,1.2f);g.drawLine(33.5f+10.5f*direction,33.5f,33.5f+4.5f*direction,27.5f,1.2f);g.drawLine(33.5f+10.5f*direction,33.5f,33.5f+4.5f*direction,39.5f,1.2f);}
        return;
    }
    if(button.getProperties().contains("panelUtility")){
        const int utility=(int)button.getProperties()["panelUtility"];
        if(utility==0){g.drawRoundedRectangle({24,24,19,12},5,1.2f);g.drawLine(30,36,30,42,1.2f);g.drawLine(37,36,37,42,1.2f);}
        else if(utility==1){juce::Path metronome;metronome.startNewSubPath(26,41);metronome.lineTo(30,25);metronome.lineTo(36,25);metronome.lineTo(41,41);metronome.closeSubPath();g.strokePath(metronome,juce::PathStrokeType(1));g.drawLine(31,37,39,23,1.1f);}
        else{g.drawEllipse(29,22,10,10,1);g.fillEllipse(32,25,4,4);text("COM",{20,34,28,13},9);}
        return;
    }
    if(key>=8&&key<16){
        const int slot=key-8;text(juce::String(slot+1),{16,13,35,20},17);
        if(slot==0||slot==1||slot>=6){const juce::StringArray legends{"IN","OUT","","","","","M1","M2"};text(legends[slot],{17,33,33,16},9);}
        else if(slot==2){g.drawRoundedRectangle({26,38,16,6},3,1);g.drawLine(34,33,34,38,1);g.drawLine(31,35,34,38,1);g.drawLine(37,35,34,38,1);}
        else if(slot==3){g.drawEllipse(24,35,11,8,1);g.drawEllipse(32,35,11,8,1);g.drawLine(25,46,42,46,.7f);}
        else if(slot==4){juce::Path reverse;reverse.startNewSubPath(37,46);reverse.lineTo(37,34);reverse.lineTo(29,34);reverse.quadraticTo(24,37,29,40);reverse.lineTo(37,40);reverse.startNewSubPath(30,40);reverse.lineTo(25,46);g.strokePath(reverse,juce::PathStrokeType(1));}
        else{for(int dot=0;dot<4;++dot)g.fillEllipse(26+dot*4.3f,37,1.8f,1.8f);g.drawLine(25,43,42,43,.8f);}
        return;
    }
    if(key>=4&&key<8){text(juce::String(key-3),{14,17,39,30},24);return;}
    if(key==0){juce::Path wave;for(int i=0;i<=64;++i){float x=21+i*25.f/64,y=33-std::sin(i/64.f*juce::MathConstants<float>::twoPi*3)*5;if(i==0)wave.startNewSubPath(x,y);else wave.lineTo(x,y);}g.strokePath(wave,juce::PathStrokeType(1.1f));}
    else if(key==1){g.drawEllipse(25,25,17,17,1);g.drawLine(33.5f,29,33.5f,47,1);g.drawLine(28,29,39,29,.8f);}
    else if(key==2){g.drawEllipse(21,28,11,11,1);g.drawEllipse(35,28,11,11,1);g.drawLine(26,39,41,39,.8f);}
    else if(key==3){for(int ch=0;ch<5;++ch){float x=24+ch*4.6f,height=ch%2?15.f:10.f;g.drawLine(x,33-height*.5f,x,33+height*.5f,1);}}
    else{text(juce::String::fromUTF8("•••"),{15,20,37,26},15);}
}
