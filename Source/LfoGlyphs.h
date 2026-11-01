#pragma once
#include <juce_graphics/juce_graphics.h>
namespace field {
// Original monoline LCD lettering. No platform font or manufacturer font data.
inline float lfoGlyphWidth(juce::juce_wchar c){switch(c){case ' ':return .45f;case '1':case 'I':return .08f;case 'M':return .9f;case 'W':return 1;case 'A':case '4':return .86f;case 'J':return .55f;case '.':return .08f;case '-':return .5f;default:return .78f;}}
inline juce::Path lfoGlyph(juce::juce_wchar c){juce::Path p;auto m=[&](float x,float y){p.startNewSubPath(x,y);};auto l=[&](float x,float y){p.lineTo(x,y);};auto q=[&](float x,float y,float xx,float yy){p.quadraticTo(x,y,xx,yy);};
 switch(c){
 case '0':case 'O':p.addRoundedRectangle(0,0,.78f,1,.34f);break;
 case '1':case 'I':m(.04f,0);l(.04f,1);break;
 case '2':m(0,.2f);q(0,0,.38f,0);q(.78f,0,.78f,.23f);q(.78f,.4f,.55f,.55f);l(0,1);l(.78f,1);break;
 case '3':m(0,.14f);q(0,0,.36f,0);q(.78f,0,.78f,.23f);q(.78f,.48f,.33f,.48f);q(.78f,.48f,.78f,.76f);q(.78f,1,.36f,1);q(0,1,0,.85f);break;
 case '4':m(.71f,1);l(.71f,0);l(0,.72f);l(.86f,.72f);break;
 case '5':m(.78f,0);l(0,0);l(0,.46f);l(.39f,.46f);q(.78f,.46f,.78f,.72f);q(.78f,1,.38f,1);q(0,1,0,.87f);break;
 case '6':m(.73f,.07f);q(.45f,-.12f,.17f,.15f);q(0,.35f,0,.72f);q(0,1,.39f,1);q(.78f,1,.78f,.72f);q(.78f,.46f,.4f,.46f);q(.04f,.46f,0,.7f);break;
 case '7':m(0,0);l(.78f,0);l(.15f,1);break;
 case '8':p.addRoundedRectangle(0,0,.78f,.48f,.26f);p.addRoundedRectangle(0,.48f,.78f,.52f,.28f);break;
 case '9':m(.05f,.93f);q(.45f,1.12f,.67f,.85f);q(.78f,.65f,.78f,.28f);q(.78f,0,.39f,0);q(0,0,0,.28f);q(0,.54f,.38f,.54f);q(.73f,.54f,.78f,.3f);break;
 case 'A':m(0,1);l(.43f,0);l(.86f,1);m(.15f,.66f);l(.71f,.66f);break;
 case 'B':m(0,1);l(0,0);l(.35f,0);q(.78f,0,.78f,.24f);q(.78f,.48f,.35f,.48f);l(0,.48f);m(.35f,.48f);q(.78f,.48f,.78f,.75f);q(.78f,1,.35f,1);l(0,1);break;
 case 'C':m(.78f,.12f);q(.6f,0,.38f,0);q(0,0,0,.5f);q(0,1,.38f,1);q(.6f,1,.78f,.88f);break;
 case 'D':m(0,0);l(0,1);l(.3f,1);q(.78f,1,.78f,.5f);q(.78f,0,.3f,0);l(0,0);break;
 case 'E':m(.78f,0);l(0,0);l(0,1);l(.78f,1);m(0,.48f);l(.65f,.48f);break;
 case 'F':m(0,1);l(0,0);l(.78f,0);m(0,.48f);l(.65f,.48f);break;
 case 'G':m(.78f,.13f);q(.6f,0,.38f,0);q(0,0,0,.5f);q(0,1,.38f,1);q(.6f,1,.78f,.85f);l(.78f,.53f);l(.43f,.53f);break;
 case 'H':m(0,0);l(0,1);m(.78f,0);l(.78f,1);m(0,.5f);l(.78f,.5f);break;
 case 'J':m(.55f,0);l(.55f,.73f);q(.55f,1,.27f,1);q(0,1,0,.77f);break;
 case 'K':m(0,0);l(0,1);m(.78f,0);l(0,.58f);m(.25f,.4f);l(.78f,1);break;
 case 'L':m(0,0);l(0,1);l(.78f,1);break;
 case 'M':m(0,1);l(0,0);l(.45f,.54f);l(.9f,0);l(.9f,1);break;
 case 'N':m(0,1);l(0,0);l(.78f,1);l(.78f,0);break;
 case 'P':m(0,1);l(0,0);l(.38f,0);q(.78f,0,.78f,.27f);q(.78f,.54f,.38f,.54f);l(0,.54f);break;
 case 'Q':p.addRoundedRectangle(0,0,.78f,1,.34f);m(.45f,.72f);l(.88f,1.1f);break;
 case 'R':m(0,1);l(0,0);l(.38f,0);q(.78f,0,.78f,.26f);q(.78f,.52f,.38f,.52f);l(0,.52f);m(.38f,.52f);l(.78f,1);break;
 case 'S':m(.75f,.12f);q(.6f,0,.38f,0);q(0,0,0,.23f);q(0,.43f,.39f,.5f);q(.78f,.57f,.78f,.77f);q(.78f,1,.39f,1);q(.1f,1,0,.87f);break;
 case 'T':m(0,0);l(.78f,0);m(.39f,0);l(.39f,1);break;
 case 'U':m(0,0);l(0,.67f);q(0,1,.39f,1);q(.78f,1,.78f,.67f);l(.78f,0);break;
 case 'V':m(0,0);l(.39f,1);l(.78f,0);break;
 case 'W':m(0,0);l(.25f,1);l(.5f,.43f);l(.75f,1);l(1,0);break;
 case 'X':m(0,0);l(.78f,1);m(.78f,0);l(0,1);break;
 case 'Y':m(0,0);l(.39f,.5f);l(.78f,0);m(.39f,.5f);l(.39f,1);break;
 case 'Z':m(0,0);l(.78f,0);l(0,1);l(.78f,1);break;
 case '-':m(0,.5f);l(.5f,.5f);break;
 case '/':m(0,1);l(.78f,0);break;
 case '.':p.addEllipse(0,.94f,.08f,.06f);break;
 default:break;
 }return p;
}
inline void drawLfoText(juce::Graphics& g,const juce::String& text,juce::Rectangle<float> rect,float height,juce::Colour colour,float thickness=1.1f){float width=0;for(auto c:text)width+=lfoGlyphWidth(c)+.17f;if(text.isNotEmpty())width-=.17f;width*=height;const float size=width>rect.getWidth()?height*rect.getWidth()/width:height;float x=rect.getCentreX()-width*(size/height)*.5f;const float y=rect.getCentreY()-size*.5f;juce::Path path;for(auto c:text){path.addPath(lfoGlyph(c),juce::AffineTransform::scale(size).translated(x,y));x+=(lfoGlyphWidth(c)+.17f)*size;}g.setColour(colour);g.strokePath(path,juce::PathStrokeType(thickness,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
}
