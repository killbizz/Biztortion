/*
  ==============================================================================

    WelcomeModule.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*

This file is part of Biztortion software.

Biztortion is free software : you can redistribute it and /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Biztortion is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Biztortion. If not, see < http://www.gnu.org/licenses/>.

*/

#include "WelcomeModule.h"

WelcomeModuleGUI::WelcomeModuleGUI()
{
    bigNoseImg = juce::ImageFileFormat::loadFrom(BinaryData::biztortionNoseAlpha_png, BinaryData::biztortionNoseAlpha_pngSize);

    author.setText("(c) 2022 Gabriel Bizzo", juce::dontSendNotification);
    author.setFont(juce::Font("Courier New", 16, 0));

    version.setText("version: 1.1", juce::dontSendNotification);
    version.setFont(juce::Font("Courier New", 16, 0));

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

}

void WelcomeModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
    g.reduceClipRegion(bigNoseImg, getTransform());
    g.setColour(juce::Colour::fromHSL((((float)(hueCounter++))*0.01f)*0.8f, 0.95f, 0.5f, 0.6f));
    g.fillAll();
    if (hueCounter == 100)
        hueCounter = 0;
}

void WelcomeModuleGUI::resized()
{
    auto area1 = getContentRenderArea();
    auto area2 = area1;

    auto topArea = area1.removeFromTop(area1.getHeight() * (1.f / 6.f));
    auto bottomArea = area2.removeFromTop(area2.getHeight() * (1.f / 3.5f));

    author.setBounds(topArea);
    author.setJustificationType(juce::Justification::centredLeft);

    version.setBounds(topArea);
    version.setJustificationType(juce::Justification::centredRight);
    
}

std::vector<juce::Component*> WelcomeModuleGUI::getAllComps()
{
    return {
        &author,
        &version
    };
}

std::vector<juce::Component*> WelcomeModuleGUI::getParamComps()
{
    return std::vector<juce::Component*>();
}

juce::AffineTransform WelcomeModuleGUI::getTransform()
{
    return juce::AffineTransform::scale( 0.85f, 0.7f).translated(94, 17);
}

void WelcomeModuleGUI::drawContainer(juce::Graphics& g)
{
    // container margin
    g.setColour(juce::Colour(132, 135, 138));
    g.drawRoundedRectangle(getContainerArea().toFloat(), 4.f, 1.f);
    g.fillRoundedRectangle(getContainerArea().toFloat(), 4.f);
    // content margin
    g.setColour(juce::Colours::black);
    auto renderArea = getContentRenderArea();
    g.drawRoundedRectangle(renderArea.toFloat(), 4.f, 1.f);
}
