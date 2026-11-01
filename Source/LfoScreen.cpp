#include "LfoScreen.h"
#include "LfoControls.h"
#include "LfoGlyphs.h"
namespace field {
void drawLfoScreen(juce::Graphics& g,juce::Rectangle<float> area,const LfoScreenState& s){
 juce::Graphics::ScopedSaveState saved(g);
 g.addTransform(juce::AffineTransform::scale(area.getWidth()/604,area.getHeight()/292).translated(area.getX(),area.getY()));
 const juce::Colour blue(0xff4a77ac),ochre(0xff8f6836),grey(0xff5a6671),red(0xffff0000),white(0xffcbd3c1),silver(0xffa1a3a5),purple(0xff5e234a),dim(0xff373d43),brown(0xff403a32),darkBlue(0xff2d3d64);
 const std::array<juce::Colour,4> caps{{darkBlue,ochre,grey,red}};
 g.setColour(juce::Colours::black);g.fillRect(0,0,604,292);
 auto label=[&](const juce::String& t,float cx,float y,juce::Colour c,float h=13,float width=160){drawLfoText(g,t,{cx-width*.5f,y,width,h},h,c,h<11?1.1f:1.6f);};
 auto number=[&](const juce::String& t,float cx,float cy,juce::Colour c,float h=31,float width=90){drawLfoText(g,t,{cx-width*.5f,cy-h*.5f,width,h},h,c,std::clamp(h*.071f,.75f,2.2f));};
 auto line=[&](float x,float y,float xx,float yy,juce::Colour c){g.setColour(c);g.drawLine(x,y,xx,yy,1.4f);};
 auto arrow=[&](float x,float y,float xx,float yy,juce::Colour c){line(x,y,xx,yy,c);const float a=std::atan2(yy-y,xx-x);juce::Path p;p.startNewSubPath(xx,yy);p.lineTo(xx-11*std::cos(a-.48f),yy-11*std::sin(a-.48f));p.lineTo(xx-11*std::cos(a+.48f),yy-11*std::sin(a+.48f));p.closeSubPath();g.setColour(c);g.fillPath(p);};
 auto pointer=[&](float cx,float y,juce::Colour c){juce::Path p;p.startNewSubPath(cx-7.5f,y);p.lineTo(cx+7.5f,y);p.lineTo(cx,y+14);p.closeSubPath();g.setColour(c);g.fillPath(p);};
 auto module=[&](int group,float x,float y){g.setColour(grey);g.fillRoundedRectangle(x,y,70,70,12);g.setColour(white);juce::Path p;
  if(group==0){p.startNewSubPath(x+11,y+45);for(int i=0;i<3;++i){const float xx=x+11+i*16;p.quadraticTo(xx+4,y+45,xx+4,y+41);p.lineTo(xx+4,y+28);p.quadraticTo(xx+4,y+24,xx+8,y+24);p.quadraticTo(xx+12,y+24,xx+12,y+28);p.lineTo(xx+12,y+42);p.quadraticTo(xx+12,y+46,xx+16,y+46);}g.strokePath(p,juce::PathStrokeType(2.3f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
  else if(group==1){p.startNewSubPath(x+11,y+47);p.lineTo(x+27,y+20);p.lineTo(x+40,y+35);p.lineTo(x+50,y+35);p.lineTo(x+60,y+47);g.strokePath(p,juce::PathStrokeType(2.3f));}
  else if(group==2){g.drawRect(x+12,y+21,46.f,28.f,2.f);drawLfoText(g,"FX",{x+16,y+26,38,20},20,white,2);}
  else{label("AMP",x+35,y+26,white,16,58);}
 };
 auto keyboard=[&](float x,float y,const juce::String& name,float captionOffset=87){g.setColour(brown);g.fillRoundedRectangle(x,y,70,70,10);g.setColour(white);g.fillRect(x+14,y+18,43.f,33.f);for(int key=1;key<4;++key){g.setColour(juce::Colours::black);g.fillRect(x+13+key*11.f,y+18,2.f,33.f);}for(int key=0;key<3;++key)g.fillRect(x+20.5f+key*11.f,y+18,8.f,22.f);label(name,x+35,y+captionOffset,dim);};
 auto amount=[&](float value,float x,float y,juce::Colour colour,int focus){juce::Path p;p.startNewSubPath(x,y);p.lineTo(x+82,y);p.lineTo(x+96,y+13);p.lineTo(x+96,y+70);p.lineTo(x,y+70);p.closeSubPath();g.setColour(s.focus==focus?colour:colour==silver?juce::Colour(0xff434341):colour==blue?juce::Colour(0xff1d242b):brown);g.strokePath(p,juce::PathStrokeType(1.4f));number(juce::String(value*100,0),x+48,y+35,colour);};
 auto knob=[&](float x,float y,int colour,bool badge){g.setColour(purple);g.drawRoundedRectangle(x,y,70,70,13,1.5f);
  g.setColour(white);g.fillEllipse(x+14,y+25,43,42);g.setColour(silver);g.fillRoundedRectangle(x+24,y+18,26,39,10);g.setColour(juce::Colour(0xffb5bab3));g.fillEllipse(x+24,y+42,26,20);g.setColour(silver);g.fillEllipse(x+24,y+34,26,18);g.setColour(white);g.fillEllipse(x+24,y+10,26,20);g.setColour(caps[(size_t)std::clamp(colour,0,3)]);g.fillEllipse(x+27,y+12,20,15);
  if(badge){g.setColour(purple);g.fillRect(x+15,y+63,42.f,17.f);label("SHIFT",x+36,y+67,silver,8,40);}
 };
 auto speaker=[&](float x,float y){g.setColour(juce::Colours::black);g.drawRect(x,y+7,10.f,15.f,2.f);juce::Path p;p.startNewSubPath(x+13,y+7);p.lineTo(x+28,y);p.lineTo(x+28,y+28);p.lineTo(x+13,y+21);p.closeSubPath();g.strokePath(p,juce::PathStrokeType(2));const float starts[]{38,42,48},controls[]{42,49,56},tops[]{8,6,3},bottoms[]{20,24,27};for(int i=0;i<3;++i){juce::Path wave;wave.startNewSubPath(x+starts[i],y+tops[i]);wave.quadraticTo(x+controls[i],y+14,x+starts[i],y+bottoms[i]);g.strokePath(wave,juce::PathStrokeType(1.5f));}};
 auto envelope=[&](float x,float y,bool volume){g.setColour(purple);g.drawRoundedRectangle(x,y,70,70,12,1.5f);juce::Path p;const float v=s.envelope;
  if(volume){p.startNewSubPath(x+12,y+58);p.quadraticTo(x+51,y+54,x+51,y+12);p.lineTo(x+51,y+58);g.setColour(purple);}
  else if(v<-.001f){p.startNewSubPath(x+11,y+12);p.quadraticTo(x+25,y+55+v*30,x+60,y+60);g.setColour(red);}
  else{p.startNewSubPath(x+11,y+60);if(v<.001f)p.lineTo(x+60,y+12);else p.quadraticTo(x+25,y+55-40*(1-v),x+60,y+12);g.setColour(red);}g.strokePath(p,juce::PathStrokeType(2));
 };
 auto speed=[&](float x,float y){g.setColour(s.focus==0?darkBlue:dim);g.fillRoundedRectangle(x,y,70,70,3);g.setColour(juce::Colours::black);g.fillEllipse(x+5,y+5,60,60);
  if(s.sync)number(juce::StringArray{"16","8","4","2","1","3/4","1/2","3/8","1/4","3/16","1/8","1/16"}[std::clamp(s.division,0,11)],x+35,y+35,blue,31,54);
  else{for(int i=0;i<8;++i){const float a=i*juce::MathConstants<float>::twoPi/8;line(x+35+std::sin(a)*24,y+35-std::cos(a)*24,x+35+std::sin(a)*29,y+35-std::cos(a)*29,darkBlue);}const float a=s.phase*juce::MathConstants<float>::twoPi;line(x+35,y+35,x+35+std::sin(a)*25,y+35-std::cos(a)*25,blue);g.setColour(blue);g.fillEllipse(x+31,y+31,8,8);number("0",x+35,y+59,blue,8,16);}
 };
 auto shape=[&](float x,float y,bool random){juce::Graphics::ScopedSaveState clip(g);const float width=random?88.f:68.f,height=random?32.f:24.f;g.reduceClipRegion(juce::Rectangle<float>(x-1,y-1,width+2,height+2).toNearestInt());juce::Path p;
  for(int point=0;point<=176;++point){const float u=point/176.f;const float value=random?s.randomHistory[(size_t)std::clamp((int)(u*8+s.phase),0,7)]:lfoWave(s.waveform==1?6:s.waveform,s.phase+u*4-(point==176?1.e-6f:0.f));const float px=x+u*width,py=y+height*.5f-value*height*.5f;if(point==0)p.startNewSubPath(px,py);else p.lineTo(px,py);}
  g.setColour(silver);g.strokePath(p,juce::PathStrokeType(1.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
 };
 auto shapeHint=[&](float x){pointer(x+35,219,darkBlue);g.setColour(darkBlue);g.fillRoundedRectangle(x,238,70,23,2);label("SHAPE",x+35,242,juce::Colours::black,13,67);};
 const int target=std::clamp(s.target,0,19),group=lfoGroup(target),parameter=lfoParameter(target);const auto parameterLabel=s.parameterLabel.isEmpty()?juce::StringArray{"PITCH","BLUE","OCHRE","GREY","ORANGE","GAIN","PAN","FILTER","ATTACK","DECAY","SUSTAIN","RELEASE","FX BLUE","FX OCHRE","FX GREY","FX RED","FX MIX","GAIN","TAPE SPEED","VOLUME"}[target]:s.parameterLabel;
 if(!s.enabled){label("LFO OFF",302,138,silver,20,230);return;}
 if(s.type==0){
  speed(58,102);arrow(128,137,171,137,darkBlue);amount(s.amount,172,102,ochre,1);shape(176,50,true);
  line(269,137,463,137,purple);module(group,328,44);line(363,114,363,164,purple);g.setColour(purple);g.fillEllipse(358.5f,132.5f,9,9);envelope(328,164,false);arrow(398,137,463,137,purple);
  line(398,79,415,79,purple);g.setColour(purple);g.fillRect(415.f,62.f,67.f,26.f);label("DEST",448.5f,68,juce::Colours::black,16,62);line(482,79,499,79,purple);line(499,79,499,102,purple);
  if(s.randomAll){label("ALL",498,80,red,10,65);for(int i=0;i<4;++i){const float x=464+(i%2)*35.f,y=104+(i/2)*35.f;g.setColour(white);g.fillEllipse(x+3,y+9,27,27);g.setColour(caps[i]);g.fillEllipse(x+9,y+4,16,13);}}else knob(463,102,parameter,false);
  pointer(498,198,red);g.setColour(red);g.fillRoundedRectangle(463,218,70,23,2);label("PARAM",498,222,juce::Colours::black,13,66);label("SPEED",93,193,s.focus==0?silver:dim);label("AMOUNT",220,193,s.focus==1?silver:dim);label("ENVELOPE",363,252,dim);
 }
 else if(s.type==1||s.type==4){const bool element=s.type==1;const float sourceX=element?83:79,amountX=element?196:190,destX=element?322:316,knobX=element?436:430;const float y=101;
  if(element){g.setColour(dim);g.fillRoundedRectangle(sourceX,100,70,70,3);g.setColour(juce::Colours::black);g.fillEllipse(sourceX+5,105,60,60);const auto source=juce::StringArray{"MIC","ENV","SUM","G"}[std::clamp(s.source,0,3)];drawLfoText(g,source,{sourceX+7,121,56,31},s.source==3?25.f:15.f,blue,1.6f);}
  else{speed(sourceX,102);shape(76,58,false);shapeHint(79);}
  arrow(sourceX+70,136,amountX,136,darkBlue);amount(s.amount,amountX,y,ochre,1);arrow(amountX+96,136,destX,136,brown);module(group,destX,y);arrow(knobX,136,destX+70,136,purple);knob(knobX,y,parameter,true);
  line(amountX+48,y,amountX+48,67,brown);line(amountX+48,67,destX+35,67,brown);arrow(destX+35,67,destX+35,y,brown);
  label(element?"SOURCE":"SPEED",sourceX+35,196,s.focus==0?silver:dim);label("AMOUNT",amountX+48,196,s.focus==1?silver:dim);label("DEST",destX+35,196,dim);label("PARAMETER",knobX+35,196,dim,13,118);label(parameterLabel,knobX+35,79,red,10,128);
 }
 else if(s.type==2){
  keyboard(37,40,"MIDI",81);arrow(107,75,145,75,brown);label("CONTINUOUS CONTROLLER",310,37,white,13,300);
  for(int i=0;i<4;++i){const float x=152+i*80.f;const int target=std::clamp(s.midiTargets[i],0,19);number(juce::String(i+1),x+35,79,i==0?blue:i==1?ochre:i==2?silver:red,26,65);line(x+35,99,x+35,111,purple);module(lfoGroup(target),x,111);line(x+35,181,x+35,194,purple);knob(x,194,lfoParameter(target),s.shifted&&(i==0||i==3));}
  line(462,146,527,146,purple);line(527,146,527,194,purple);g.setColour(purple);g.fillRect(494.f,174.f,67.f,26.f);label("DEST",527.5f,180,juce::Colours::black,16,63);
 }
 else if(s.type==3){
  speed(63,108);arrow(133,143,172,143,dim);line(172,143,172,85,dim);line(172,143,172,203,dim);line(172,85,218,85,dim);line(172,203,218,203,dim);
  amount(s.pitch,218,50,ochre,1);amount(s.volume,218,168,silver,2);line(314,85,350,85,brown);line(314,203,350,203,brown);envelope(350,50,false);envelope(350,168,true);line(420,85,456,85,dim);line(420,203,456,203,dim);
  g.setColour(dim);g.fillRoundedRectangle(456,50,70,70,12);g.fillRoundedRectangle(456,168,70,70,12);g.setColour(juce::Colours::black);g.fillEllipse(484,92,10,11);g.fillRect(492.f,70.f,3.f,28.f);speaker(465,188);
  label("SPEED",98,196,s.focus==0?silver:dim);label("AMOUNT",266,136,dim);label("ENVELOPE",385,136,dim);label("DEST",491,136,dim);shapeHint(65);
 }
 else if(s.type==5){
  keyboard(63,108,"VELOCITY");line(133,143,172,143,brown);line(172,143,172,84,brown);line(172,143,172,202,brown);line(172,84,218,84,brown);line(172,202,218,202,brown);
  amount(s.amount,218,49,ochre,0);amount(s.velocityVolume,218,167,blue,1);line(314,84,349,84,brown);line(314,202,349,202,brown);module(group,349,49);g.setColour(dim);g.fillRoundedRectangle(349,167,70,70,12);speaker(358,187);
  arrow(456,84,419,84,purple);knob(456,49,parameter,true);label(parameterLabel,491,24,red,10,128);label("AMOUNT",266,133,dim);label("DEST",384,133,dim);label("PARAMETER",491,133,dim,13,120);
 }
}
}
