#include "DimensionScreen.h"
#include <cmath>

namespace field {
void drawDimensionScreen(juce::Graphics& g,juce::Rectangle<float> bounds,float waveform,float stereo,float cutoff,float resonance,double sampleRate,float phase){
    juce::Graphics::ScopedSaveState state(g);
    g.reduceClipRegion(bounds.toNearestInt());
    g.addTransform(juce::AffineTransform::scale(bounds.getWidth()/604.f,bounds.getHeight()/292.f).translated(bounds.getX(),bounds.getY()));
    const juce::Colour blue{0xff4476aa},dimBlue{0xff2d4c75},ochre{0xffb68d42},grey{0xffb5bfaf},muted{0xff607582},red{0xffff1810},grid{0xff343229};
    g.fillAll(juce::Colours::black);
    auto text=[&](const juce::String& value,juce::Rectangle<float> area,float size,juce::Colour colour,int justification=juce::Justification::centredLeft){g.setColour(colour);g.setFont(juce::Font(juce::FontOptions(size)));g.drawText(value,area,justification,true);};
    waveform=juce::jlimit(0.f,1.f,waveform);stereo=juce::jlimit(0.f,1.f,stereo);cutoff=juce::jlimit(0.f,1.f,cutoff);resonance=juce::jlimit(0.f,1.f,resonance);
    text("WAVEFORM",{20,12,114,24},17,dimBlue);
    text("STEREO",{143,12,80,24},17,ochre);
    g.setColour(ochre.withAlpha(.5f));g.drawLine(225,25,290,25,1.2f);g.setColour(muted);g.drawLine(257,19,257,31,1);
    const float marker=225+stereo*65;g.setColour(ochre);g.drawLine(marker,21,marker,29,2);
    text("FILTER FREQ",{302,12,116,24},17,muted);
    // Same unmodulated cutoff law as Engine::voice; never hard-code the guide's
    // example 440–1440 readout as if it were the current hardware response.
    const float hz=std::min((float)(sampleRate*.4),100.f+cutoff*cutoff*18000.f);
    text(hz>=1000?juce::String(hz/1000,1)+"k Hz":juce::String(hz,0)+" Hz",{420,12,121,24},18,grey);
    text("RES",{545,12,45,24},17,red);

    constexpr float left=135,right=540,top=41,bottom=253;
    g.setColour(grid);for(int line=0;line<7;++line){float y=top+(bottom-top)*line/6;g.drawLine(left,y,577,y,.65f);g.setColour(grey.withAlpha(.6f));g.drawLine(left-12,y,left,y,.8f);g.setColour(grid);}
    g.setColour(grey.withAlpha(.4f));g.drawLine(left,top,left,bottom,.8f);
    for(float x:{220.f,320.f,420.f,540.f}){g.setColour(muted.withAlpha(.38f));for(float y=top;y<bottom;y+=12)g.drawLine(x,y,x,juce::jmin(y+7,bottom),.8f);}
    text("-1",{206,255,30,21},16,muted);text("0",{313,255,22,21},16,muted);text("1",{413,255,22,21},16,muted);

    const int selected=juce::jlimit(0,5,(int)(waveform*5.999f));
    for(int row=0;row<6;++row){
        const float y=46+row*35.f;
        if(row==selected){g.setColour(blue.withAlpha(.21f));g.fillRect(21.f,y-6,114.f,35.f);g.setColour(blue);g.drawRect(21.f,y-6,114.f,35.f,1.2f);}
        g.setColour(row==selected?blue:dimBlue);
        if(row==0||row==5){g.drawRoundedRectangle({27,y-1,68,24},2,1.1f);text("NOISE",{33,y-2,64,25},18,row==selected?blue:dimBlue);continue;}
        juce::Path shape;
        if(row==1){shape.startNewSubPath(24,y+23);for(int pulse=0;pulse<2;++pulse){float x=41+pulse*33.f;shape.lineTo(x,y+23);shape.lineTo(x,y+1);shape.quadraticTo(x+3,y-3,x+6,y+1);shape.lineTo(x+6,y+23);}shape.lineTo(96,y+23);}
        else if(row==2){shape.startNewSubPath(24,y+23);shape.lineTo(42,y+23);shape.lineTo(42,y);shape.lineTo(59,y);shape.lineTo(59,y+23);shape.lineTo(76,y+23);shape.lineTo(76,y);shape.lineTo(95,y);}
        else{shape.startNewSubPath(26,y+23);for(int tooth=0;tooth<3;++tooth){float x=26+tooth*24.f;shape.lineTo(x+20,y);if(tooth<2)shape.lineTo(x+20,y+23);}if(row==4){g.strokePath(shape,juce::PathStrokeType(1.2f));g.drawRoundedRectangle({69,y+6,39,17},2,1);text("SUB",{72,y+5,34,18},13,blue);continue;}}
        g.strokePath(shape,juce::PathStrokeType(1.2f));
    }
    {juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(juce::Rectangle<float>(left,top,right-left,bottom-top).toNearestInt());
        // Smooth, animated XY-style control preview. Width follows Stereo;
        // waveform selection changes the curve, without claiming a live scope.
        for(int trace=0;trace<4;++trace){juce::Path path;const float offset=trace*.075f;
            for(int i=0;i<=360;++i){const float t=i/360.f*juce::MathConstants<float>::twoPi;
                const float x=320+std::sin(t+offset)*(55+stereo*72);
                const float y=155+std::sin(t+(trace%2?1.f:-1.f)*(.35f+stereo*1.1f)+offset+selected*.035f+std::sin(phase)*.12f)*(45+stereo*14);
                if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);
            }
            g.setColour((trace%2?blue:dimBlue).withAlpha(trace==0?.9f:.65f));g.strokePath(path,juce::PathStrokeType(1.2f));
        }
    }
    // The orange control is visualized using the reference's resonance shape;
    // the independent audio engine's macro law has not been hardware matched.
    const float height=12+resonance*92,centre=558+cutoff*5,width=7+resonance*12;
    juce::Path peak;peak.startNewSubPath(543,bottom);
    for(int i=0;i<=70;++i){float x=543+i/70.f*40.f;peak.lineTo(x,bottom-height*std::exp(-std::pow((x-centre)/width,2)*2));}
    peak.lineTo(583,bottom);peak.closeSubPath();g.setColour(red);g.fillPath(peak);
}
}
