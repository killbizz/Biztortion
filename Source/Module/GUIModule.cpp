/*
  ==============================================================================

    GUIModule.cpp

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

#include "GUIModule.h"

#include "../Shared/GUIStuff.h"

GUIModule::GUIModule() : remapper(0.f, 100.f, 1.f, 1.3f) // exponential remapping for a smoother lightness fx in the outline
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(45, 70); // define the range

    brightnessCounter = distr(gen);

    moduleHelpButton.setVisible(false);
    moduleHelpButton.setTooltip("About this module");
    moduleHelpButton.setColour(juce::TextButton::buttonColourId, BiztortionTheme::surfaceRaised);
    moduleHelpButton.setColour(juce::TextButton::textColourOffId, BiztortionTheme::text);
    addAndMakeVisible(moduleHelpButton);

    auto safePtr = juce::Component::SafePointer<GUIModule>(this);
    moduleHelpButton.onClick = [safePtr]() {
        if (auto* module = safePtr.getComponent())
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::NoIcon,
                module->moduleHelpTitle,
                module->moduleHelpDescription,
                "Got it",
                module);
    };

    startTimerHz(10);
}

void GUIModule::drawContainer(juce::Graphics& g)
{
    applyModuleAccentToSliders();

    auto container = getContainerArea().toFloat();
    g.setColour(BiztortionTheme::surface);
    g.fillRoundedRectangle(container, 7.0f);
    g.setColour(BiztortionTheme::border);
    g.drawRoundedRectangle(container, 7.0f, 1.0f);

    auto accent = container.removeFromLeft(4.0f).reduced(0.0f, 10.0f);
    g.setColour(moduleColor);
    g.fillRoundedRectangle(accent, 2.0f);

    if (moduleHelpButton.isVisible())
    {
        const auto content = getContentRenderArea();
        moduleHelpButton.setBounds(content.getRight() - 26, content.getY() + 4, 24, 24);
        moduleHelpButton.toFront(false);
    }

}

void GUIModule::setModuleHelp(juce::String title, juce::String description)
{
    moduleHelpTitle = std::move(title);
    moduleHelpDescription = std::move(description);
    moduleHelpButton.setVisible(true);
}

juce::Rectangle<int> GUIModule::getContentRenderArea()
{
    auto bounds = getContainerArea();
    bounds.reduce(10, 10);

    return bounds;
}

void GUIModule::handleParamCompsEnablement(bool bypass)
{
    for (auto* comp : getParamComps())
    {
        comp->setEnabled(!bypass);
    }
}

void GUIModule::paint(juce::Graphics& g)
{
    applyModuleAccentToSliders();
    drawContainer(g);
}

void GUIModule::applyModuleAccentToSliders()
{
    const auto accent = moduleColor.isTransparent()
        ? BiztortionTheme::controlAccent
        : moduleColor;

    for (int index = 0; index < getNumChildComponents(); ++index)
        if (auto* slider = dynamic_cast<juce::Slider*>(getChildComponent(index)))
            if (!slider->isColourSpecified(juce::Slider::thumbColourId))
                slider->setColour(juce::Slider::thumbColourId, accent);
}

void GUIModule::timerCallback()
{
    repaint();
}

juce::Rectangle<int> GUIModule::getContainerArea()
{
    // returns a dimesion reduced rectangle as bounds in order to avoid margin collisions
    auto bounds = getLocalBounds();
    bounds.reduce(5, 5);

    return bounds;
}