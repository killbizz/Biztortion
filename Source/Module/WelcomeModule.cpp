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

    /*title.setText("Biztortion", juce::dontSendNotification);
    title.setFont(juce::Font("Prestige Elite Std", 28, 0));*/

    author.setText("(c) 2021 Gabriel Bizzo", juce::dontSendNotification);
    author.setFont(juce::Font("Courier New", 16, 0));

    version.setText("version: 1.0", juce::dontSendNotification);
    version.setFont(juce::Font("Courier New", 16, 0));

    auto buildDate = juce::Time::getCompilationDate().toString(true, false);

    buildInfo.setText("Build: " + buildDate, juce::dontSendNotification);
    buildInfo.setFont(juce::Font("Courier New", 16, 0));


    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    startTimerHz(45);

}

void WelcomeModuleGUI::timerCallback()
{
    repaint();
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
    auto welcomeArea = getContentRenderArea();

    auto titleArea = welcomeArea.removeFromTop(welcomeArea.getHeight() * (1.f / 6.f));

    title.setBounds(titleArea);
    title.setJustificationType(juce::Justification::centred);

    author.setBounds(titleArea);
    author.setJustificationType(juce::Justification::centredLeft);

    version.setBounds(titleArea);
    version.setJustificationType(juce::Justification::centredRight);

    buildInfo.setBounds(welcomeArea);
    buildInfo.setJustificationType(juce::Justification::topRight);
}

std::vector<juce::Component*> WelcomeModuleGUI::getAllComps()
{
    return {
        //&title,
        &author,
        &version,
        &buildInfo
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
