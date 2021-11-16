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
    bigNose = juce::Drawable::createFromImageData(BinaryData::biztortionNoseAlpha_png, BinaryData::biztortionNoseAlpha_pngSize);
    bigNose->setAlpha(0.30);
    addAndMakeVisible(*bigNose);

    title.setText("Welcome to Biztortion", juce::dontSendNotification);
    title.setFont(juce::Font("Prestige Elite Std", 38, 0));

    one.setFont(juce::Font("Marlett", 36, juce::Font::bold));
    one.setText("a", juce::dontSendNotification);

    firstRow.setText("Use the \"+\" button to create modules that distort the signal beyond your imagination", juce::dontSendNotification);
    firstRow.setFont(juce::Font("Prestige Elite Std", 22, 0));


    two.setFont(juce::Font("Marlett", 36, juce::Font::bold));
    two.setText("a", juce::dontSendNotification);

    secondRow.setText("Chain modules together to achieve unique effects in your music", juce::dontSendNotification);
    secondRow.setFont(juce::Font("Prestige Elite Std", 22, 0));

    three.setFont(juce::Font("Marlett", 36, juce::Font::bold));
    three.setText("a", juce::dontSendNotification);

    thirdRow.setText("Use the \"x\" button to remove a module from the chain and the power button in the module section to bypass the current module", juce::dontSendNotification);
    thirdRow.setFont(juce::Font("Prestige Elite Std", 22, 0));

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
}

void WelcomeModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
    bigNose->setCentreRelative(0.5f, 0.5f);
    //bigNose->draw(g, 1.f, getTransform());
}

void WelcomeModuleGUI::resized()
{
    bigNose->setTransform(getTransform());
    bigNose->setCentreRelative(0.5f, 0.5f);
    
    auto welcomeArea = getContentRenderArea();

    auto titleArea = welcomeArea.removeFromTop(welcomeArea.getHeight() * (1.f / 5.f));
    auto numbersArea = welcomeArea.removeFromLeft(welcomeArea.getWidth() * (1.f / 10.f));

    auto oneArea = numbersArea.removeFromTop(numbersArea.getHeight() * (1.f / 3.f));
    auto twoArea = numbersArea.removeFromTop(numbersArea.getHeight() * (1.f / 2.f));
    auto threeArea = numbersArea;

    auto firstRowArea = welcomeArea.removeFromTop(welcomeArea.getHeight() * (1.f / 3.f));
    auto secondRowArea = welcomeArea.removeFromTop(welcomeArea.getHeight() * (1.f / 2.f));
    auto thirdRowArea = welcomeArea;

    title.setBounds(titleArea);
    title.setJustificationType(juce::Justification::centred);

    one.setBounds(oneArea);
    one.setJustificationType(juce::Justification::centred);

    two.setBounds(twoArea);
    two.setJustificationType(juce::Justification::centred);

    three.setBounds(threeArea);
    three.setJustificationType(juce::Justification::centred);

    firstRow.setBounds(firstRowArea);
    firstRow.setJustificationType(juce::Justification::centredLeft);

    secondRow.setBounds(secondRowArea);
    secondRow.setJustificationType(juce::Justification::centredLeft);

    thirdRow.setBounds(thirdRowArea);
    thirdRow.setJustificationType(juce::Justification::centredLeft);
}

std::vector<juce::Component*> WelcomeModuleGUI::getComps()
{
    return {
        &*bigNose,
        &title,
        &one,
        &two,
        &three,
        &firstRow,
        &secondRow,
        &thirdRow
    };
}

juce::AffineTransform WelcomeModuleGUI::getTransform()
{
    return juce::AffineTransform::scale( 0.85f, 0.7f);
}
