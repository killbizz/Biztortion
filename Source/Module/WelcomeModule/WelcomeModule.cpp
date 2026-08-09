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

#include "../../Shared/GUIStuff.h"

WelcomeModuleGUI::WelcomeModuleGUI()
{
    bigNoseImg = juce::ImageFileFormat::loadFrom(BinaryData::biztortionNoseAlpha_png, BinaryData::biztortionNoseAlpha_pngSize);

    author.setText("(c) 2026 Gabriel Bizzo", juce::dontSendNotification);
    author.setFont(juce::Font("Segoe UI", 14, juce::Font::plain));
    author.setColour(juce::Label::textColourId, BiztortionTheme::textMuted);

    version.setText("version: 2.0", juce::dontSendNotification);
    version.setFont(juce::Font("Segoe UI", 14, juce::Font::plain));
    version.setColour(juce::Label::textColourId, BiztortionTheme::textMuted);

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, 100); // define the range

    hueCounter = distr(gen);

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    if (isTimerRunning())
        stopTimer();

    startTimerHz(40);

}

void WelcomeModuleGUI::paint(juce::Graphics& g)
{
    const auto pulse = 0.5f + 0.5f * std::sin((float) hueCounter++ * 0.055f);
    const auto brightness = juce::jmap(pulse, 0.58f, 1.0f);
    moduleColor = juce::Colour::fromFloatRGBA(brightness, brightness, brightness, 0.92f);
    drawContainer(g);

    {
        juce::Graphics::ScopedSaveState state(g);
        g.addTransform(getTransform());
        juce::DropShadow(juce::Colours::white.withAlpha(0.16f + pulse * 0.28f),
                         juce::roundToInt(8.0f + pulse * 8.0f), {})
            .drawForImage(g, bigNoseImg);
    }

    g.reduceClipRegion(bigNoseImg, getTransform());
    g.setColour(moduleColor);
    g.fillAll();
    if (hueCounter == 100000)
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
    if (!bigNoseImg.isValid())
        return {};

    auto target = getContentRenderArea().toFloat().reduced(42.0f, 30.0f);
    target.removeFromTop(18.0f);

    const auto scale = juce::jmin(target.getWidth() / bigNoseImg.getWidth(),
                                  target.getHeight() / bigNoseImg.getHeight()) * 0.82f;
    const auto scaledWidth = bigNoseImg.getWidth() * scale;
    const auto scaledHeight = bigNoseImg.getHeight() * scale;
    const auto translationX = target.getCentreX() - scaledWidth * 0.5f;
    const auto translationY = target.getCentreY() - scaledHeight * 0.5f;

    return juce::AffineTransform::scale(scale).translated(translationX, translationY);
}

void WelcomeModuleGUI::drawContainer(juce::Graphics& g)
{
    auto container = getContainerArea().toFloat();
    g.setColour(BiztortionTheme::surface);
    g.fillRoundedRectangle(container, 7.0f);
    g.setColour(BiztortionTheme::border);
    g.drawRoundedRectangle(container, 7.0f, 1.0f);
}
