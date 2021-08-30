/*
  ==============================================================================

    SingleModule.h
    Created: 30 Aug 2021 10:59:59am
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SingleModule : public juce::Component {
public:
    void drawContainer(juce::Graphics& g);
    virtual std::vector<juce::Component*> getComps() = 0;
    juce::Rectangle<int> getContentRenderArea();
private:
    juce::Rectangle<int> getContainerArea();
};