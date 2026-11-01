#include "DimensionScreen.h"
#include <cmath>

namespace field {
void drawDimensionScreen(juce::Graphics& g,juce::Rectangle<float> bounds,float waveform,float stereo,float cutoff,float resonance,double sampleRate,float phase){
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(bounds.toNearestInt());
    g.addTransform(juce::AffineTransform::scale(bounds.getWidth()/604.f,bounds.getHeight()/292.f).translated(bounds.getX(),bounds.getY()));
    const juce::Colour blue{0xff4a77ac},dimBlue{0xff2d3d64},ochre{0xff8f6836},grey{0xffc4c8b6},muted{0xff5a6671},red{0xffff0000},grid{0xff2a221c};
    g.fillAll(juce::Colours::black);
    auto text=[&](const juce::String& value,juce::Rectangle<float> area,float size,juce::Colour colour,int justification=juce::Justification::centredLeft){g.setColour(colour);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(value,area,justification,true);};
    waveform=juce::jlimit(0.f,1.f,waveform);stereo=juce::jlimit(0.f,1.f,stereo);cutoff=juce::jlimit(0.f,1.f,cutoff);resonance=juce::jlimit(0.f,1.f,resonance);
    text("WAVEFORM",{20,12,114,24},19,dimBlue);
    text("STEREO",{140,12,80,24},19,ochre);
    g.setColour(ochre.withAlpha(.5f));g.drawLine(225,25,290,25,1.2f);g.setColour(muted);g.drawLine(257,19,257,31,1);
    const float marker=225+stereo*65;g.setColour(ochre);g.drawLine(marker,21,marker,29,2);
    text("FILTER FREQ",{300,12,116,24},19,muted);
    // Same unmodulated cutoff law as Engine::voice; never hard-code the guide's
    // example 440–1440 readout as if it were the current hardware response.
    const float hz=std::min((float)(sampleRate*.4),100.f+cutoff*cutoff*18000.f);
    text(hz>=1000?juce::String(hz/1000,1)+"k Hz":juce::String(hz,0)+" Hz",{420,12,121,24},18,grey);
    text("RES",{545,12,45,24},19,red);

    constexpr float left=132,right=506,top=39,bottom=252;
    g.setColour(grid);for(int line=0;line<7;++line){float y=top+(bottom-top)*line/6;g.drawLine(left,y,506,y,1.f);g.setColour(grey.withAlpha(.85f));g.drawLine(left-12,y,left,y,.8f);g.setColour(grid);}
    g.setColour(grey.withAlpha(.8f));g.drawLine(left,top,left,bottom,.8f);
    for(float x:{220.f,418.f,506.f}){g.setColour(muted.withAlpha(.38f));for(float y=top;y<bottom;y+=12)g.drawLine(x,y,x,juce::jmin(y+7,bottom),.8f);}
    text("-1",{206,255,30,21},16,muted);text("0",{313,255,22,21},16,muted);text("1",{413,255,22,21},16,muted);

    const int selected=juce::jlimit(0,5,(int)(waveform*5.999f));
    for(int row=0;row<6;++row){
        const float y=46+row*35.f;
        if(row==selected){g.setColour(juce::Colour(0xff0f1b27));g.fillRect(21.f,y-6,112.f,35.f);g.setColour(blue);g.drawRect(21.f,y-6,112.f,35.f,1.2f);}
        g.setColour(row==selected?blue:dimBlue);
        if(row==0||row==5){g.drawRoundedRectangle({27,y-1,68,24},2,1.1f);text("NOISE",{33,y-2,64,25},18,row==selected?blue:dimBlue);continue;}
        juce::Path shape;
        if(row==1){shape.startNewSubPath(24,y+23);for(int pulse=0;pulse<2;++pulse){float x=41+pulse*33.f;shape.lineTo(x,y+23);shape.lineTo(x,y+1);shape.quadraticTo(x+3,y-3,x+6,y+1);shape.lineTo(x+6,y+23);}shape.lineTo(96,y+23);}
        else if(row==2){shape.startNewSubPath(24,y+23);shape.lineTo(42,y+23);shape.lineTo(42,y);shape.lineTo(59,y);shape.lineTo(59,y+23);shape.lineTo(76,y+23);shape.lineTo(76,y);shape.lineTo(95,y);}
        else{shape.startNewSubPath(26,y+23);for(int tooth=0;tooth<3;++tooth){float x=26+tooth*24.f;shape.lineTo(x+20,y);if(tooth<2)shape.lineTo(x+20,y+23);}if(row==4){g.strokePath(shape,juce::PathStrokeType(1.2f));g.setColour(blue);g.drawRoundedRectangle({69,y+6,39,17},2,1);text("SUB",{72,y+5,34,18},13,blue);continue;}}
        g.strokePath(shape,juce::PathStrokeType(1.2f));
    }
    {juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(juce::Rectangle<float>(left,top,right-left,bottom-top).toNearestInt());
        // Smooth, animated XY-style control preview. Width follows Stereo;
        // waveform selection changes the curve, without claiming a live scope.
        for(int trace=0;trace<2;++trace){juce::Path path;
            // The guide's knot is a paired 2:3 Lissajous, not four ellipses.
            const float width=(80+stereo*60)*(trace?.8795f:1.f),height=(40+stereo*14)*(trace?.8755f:1.f);
            for(int i=0;i<=720;++i){const float t=i/720.f*juce::MathConstants<float>::twoPi;
                const float x=318.53f+std::sin(t*2)*width;
                const float y=140.65f+std::sin(t*3+juce::MathConstants<float>::pi+(selected-1)*.035f+std::sin(phase)*.04f)*height;
                if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);
            }
            g.setColour(trace?blue:dimBlue);g.strokePath(path,juce::PathStrokeType(1.5f));
        }
    }
    g.setColour(grid);g.drawLine(584,top,584,bottom,1);for(int i=0;i<7;++i){float y=top+(bottom-top)*i/6;g.drawLine(572,y,584,y,1);}
    // The orange control is visualized using the reference's resonance shape;
    // the independent audio engine's macro law has not been hardware matched.
    const float height=16+resonance*96,centre=562+cutoff*5,width=7+resonance*12;
    juce::Path peak;peak.startNewSubPath(543,bottom);
    for(int i=0;i<=70;++i){float x=543+i/70.f*40.f;peak.lineTo(x,bottom-height*std::exp(-std::pow((x-centre)/width,4)*2));}
    peak.lineTo(583,bottom);peak.closeSubPath();g.setColour(red);g.fillPath(peak);
}
}
