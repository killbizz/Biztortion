/*
  ==============================================================================

    GUIModule.h
    Created: 30 Aug 2021 10:59:59am
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class GUIModule : public juce::Component {
public:
    GUIModule(unsigned int gp);
    unsigned int getGridPosition();
    void setGridPosition(unsigned int gp);
    void drawContainer(juce::Graphics& g);
    virtual std::vector<juce::Component*> getComps() = 0;
    juce::Rectangle<int> getContentRenderArea();
private:
    unsigned int gridPosition;

    juce::Rectangle<int> getContainerArea();
};