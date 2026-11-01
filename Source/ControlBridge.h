#pragma once
#include <juce_core/juce_core.h>
#include <functional>
#include <deque>
class ControlBridge {
public:
 using Handler=std::function<juce::var(const juce::String&,const juce::var&)>;
 explicit ControlBridge(Handler,juce::File root={});
 ~ControlBridge();
 void poll();
 juce::File descriptor() const {return directory.getChildFile("instance.json");}
private:
 void heartbeat();
 Handler handler;juce::File directory;juce::String id,token;int ticks=0;std::deque<juce::String> seen;
};
