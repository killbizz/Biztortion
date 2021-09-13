/*
  ==============================================================================

    WelcomeModule.cpp
    Created: 13 Sep 2021 2:59:45pm
    Author:  gabri

  ==============================================================================
*/

#include "WelcomeModule.h"

WelcomeModuleGUI::WelcomeModuleGUI()
{
    bigNose = juce::Drawable::createFromImageData(BinaryData::bigNose1_svg, BinaryData::bigNose1_svgSize);
    addAndMakeVisible(*bigNose);
}

void WelcomeModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
    //bigNose->draw(g, 1.f, getTransform());
}

void WelcomeModuleGUI::resized()
{
    bigNose->setTransform(getTransform());
    bigNose->setCentreRelative(0.5f, 0.5f);
    //bigNose->setBounds(getContentRenderArea());
}

std::vector<juce::Component*> WelcomeModuleGUI::getComps()
{
    return {
        &*bigNose
    };
}

juce::AffineTransform WelcomeModuleGUI::getTransform()
{
    return juce::AffineTransform::scale(0.7f, 0.45f);
}
