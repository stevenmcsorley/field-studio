#pragma once
#include "MixerScreen.h"
namespace field {
// Original single-stroke numerals, independent of platform font substitution.
inline void lcdNumeral(juce::Graphics& g,juce::String value,juce::Rectangle<float> bounds,juce::Colour colour,float thickness=2.f){
 const float advance=43,total=juce::jmax(1,value.length())*advance-4;const float sx=bounds.getWidth()/std::max(82.f,total),sy=bounds.getHeight()/62;
 juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::scale(sx,sy).translated(bounds.getCentreX()-total*sx*.5f,bounds.getY()));g.setColour(colour);
 for(int index=0;index<value.length();++index){juce::Path p;const auto c=value[index];
 switch(c){
 case '0':p.addRoundedRectangle(1,1,37,59,17,17);break;
 case '1':p.startNewSubPath(17,1);p.lineTo(17,60);break;
 case '2':p.startNewSubPath(1,14);p.cubicTo(1,-4,38,-4,38,15);p.cubicTo(38,29,13,35,1,60);p.lineTo(39,60);break;
 case '3':p.startNewSubPath(1,2);p.lineTo(22,2);p.cubicTo(44,2,43,28,23,29);p.cubicTo(45,29,44,60,23,60);p.lineTo(0,60);break;
 case '4':p.startNewSubPath(31,60);p.lineTo(31,1);p.lineTo(0,37);p.lineTo(40,37);break;
 case '5':p.startNewSubPath(38,1);p.lineTo(2,1);p.lineTo(2,28);p.lineTo(23,28);p.cubicTo(46,28,44,60,23,60);p.lineTo(1,60);break;
 case '6':p.startNewSubPath(35,1);p.lineTo(19,1);p.cubicTo(-1,1,1,27,1,44);p.cubicTo(1,66,39,66,39,44);p.cubicTo(39,26,1,26,1,44);break;
 case '7':p.startNewSubPath(0,1);p.lineTo(39,1);p.lineTo(10,60);break;
 case '8':p.addRoundedRectangle(2,1,36,27,13,13);p.addRoundedRectangle(0,29,40,31,14,14);break;
 case '9':p.startNewSubPath(3,60);p.lineTo(19,60);p.cubicTo(39,60,39,33,39,17);p.cubicTo(39,-5,1,-5,1,17);p.cubicTo(1,35,39,35,39,17);break;
 case '-':p.startNewSubPath(9,31);p.lineTo(30,31);break;
 case '.':p.addEllipse(16,57,2,2);break;
 default:break;
 }
 g.strokePath(p,juce::PathStrokeType(thickness,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));g.addTransform(juce::AffineTransform::translation(advance,0));
 }
}
}
