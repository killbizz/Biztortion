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

    /*title.setText("Biztortion", juce::dontSendNotification);
    title.setFont(juce::Font("Prestige Elite Std", 28, 0));*/

    author.setText("(c) 2021 Gabriel Bizzo", juce::dontSendNotification);
    author.setFont(juce::Font("Prestige Elite Std", 14, 0));

    version.setText("v: 1.0", juce::dontSendNotification);
    version.setFont(juce::Font("Prestige Elite Std", 14, 0));

    one.setFont(juce::Font("Marlett", 26, juce::Font::bold));
    one.setText("a", juce::dontSendNotification);

    firstRow.setText("Biztortion is an open-source modular distortion plugin, click on the \"About\" button for more info", juce::dontSendNotification);
    firstRow.setFont(juce::Font("Prestige Elite Std", 18, 0));


    two.setFont(juce::Font("Marlett", 26, juce::Font::bold));
    two.setText("a", juce::dontSendNotification);

    secondRow.setText("Use the slots below to link modules and create your own custom processing chain", juce::dontSendNotification);
    secondRow.setFont(juce::Font("Prestige Elite Std", 18, 0));

    three.setFont(juce::Font("Marlett", 26, juce::Font::bold));
    three.setText("a", juce::dontSendNotification);

    thirdRow.setText("If you need help with some module functionalities click on the \"?\" button to enable tooltips", juce::dontSendNotification);
    thirdRow.setFont(juce::Font("Prestige Elite Std", 18, 0));

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

    auto titleArea = welcomeArea.removeFromTop(welcomeArea.getHeight() * (1.f / 6.f));
    auto numbersArea = welcomeArea.removeFromLeft(welcomeArea.getWidth() * (1.f / 10.f));

    auto oneArea = numbersArea.removeFromTop(numbersArea.getHeight() * (1.f / 3.f));
    auto twoArea = numbersArea.removeFromTop(numbersArea.getHeight() * (1.f / 2.f));
    auto threeArea = numbersArea;

    auto firstRowArea = welcomeArea.removeFromTop(welcomeArea.getHeight() * (1.f / 3.f));
    auto secondRowArea = welcomeArea.removeFromTop(welcomeArea.getHeight() * (1.f / 2.f));
    auto thirdRowArea = welcomeArea;

    title.setBounds(titleArea);
    title.setJustificationType(juce::Justification::centred);

    author.setBounds(titleArea);
    author.setJustificationType(juce::Justification::centredLeft);

    version.setBounds(titleArea);
    version.setJustificationType(juce::Justification::centredRight);

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
        //&title,
        &author,
        &version,
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
