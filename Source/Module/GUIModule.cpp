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
    moduleHelpButton.setLookAndFeel(&moduleHelpButtonLookAndFeel);
    addAndMakeVisible(moduleHelpButton);

    auto safePtr = juce::Component::SafePointer<GUIModule>(this);
    moduleHelpButton.onClick = [safePtr]() {
        if (auto* module = safePtr.getComponent())
            module->showModuleHelpWindow();
    };

    startTimerHz(10);
}

GUIModule::~GUIModule()
{
    // the look and feel is a member, so detach it before it goes out of scope
    moduleHelpButton.setLookAndFeel(nullptr);
}

// Mirrors the plugin's "About Biztortion" dialog (see BiztortionAudioProcessorEditor)
// so every help window in the app shares one style.
void GUIModule::showModuleHelpWindow()
{
    constexpr int contentWidth = 460;

    moduleHelpComponent = std::make_unique<HelpComponent>(moduleHelpDescription);
    moduleHelpComponent->setBounds(0, 0, contentWidth,
        moduleHelpComponent->getPreferredHeightForWidth(contentWidth));

    moduleHelpWindow = std::make_unique<juce::AlertWindow>(moduleHelpTitle, "",
        juce::MessageBoxIconType::NoIcon);
    moduleHelpWindow->setLookAndFeel(&helpWindowLookAndFeel);
    moduleHelpWindow->addButton("Got it!", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
    moduleHelpWindow->addCustomComponent(moduleHelpComponent.get());

    moduleHelpWindow->setColour(juce::AlertWindow::backgroundColourId, BiztortionTheme::surface);
    moduleHelpWindow->setColour(juce::AlertWindow::textColourId, BiztortionTheme::text);
    moduleHelpWindow->setColour(juce::AlertWindow::outlineColourId, BiztortionTheme::border);

    auto safePtr = juce::Component::SafePointer<GUIModule>(this);
    moduleHelpWindow->enterModalState(true,
        juce::ModalCallbackFunction::create([safePtr](int) {
            if (auto* module = safePtr.getComponent())
            {
                // the window owns the custom component, so tear both down together
                module->moduleHelpWindow->setLookAndFeel(nullptr);
                module->moduleHelpWindow.reset();
                module->moduleHelpComponent.reset();
            }
        }));
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

    drawParameterGroups(g);

    if (moduleHelpButton.isVisible())
    {
        // same size as the bypass button on the opposite side of the header, and vertically
        // centred on the same 36px header strip the modules lay out in resized()
        const auto content = getContentRenderArea();
        moduleHelpButton.setBounds(juce::Rectangle<int>(headerButtonSize, headerButtonSize)
            .withCentre({ content.getRight() - headerButtonGap - headerButtonSize / 2,
                          content.getY() + 18 }));
        moduleHelpButton.toFront(false);
    }

}

void GUIModule::clearParameterGroups()
{
    parameterGroups.clear();
}

void GUIModule::addParameterGroup(juce::Rectangle<int> area)
{
    if (!area.isEmpty())
        parameterGroups.push_back(area);
}

void GUIModule::drawParameterGroups(juce::Graphics& g)
{
    if (parameterGroups.empty())
        return;

    const auto accent = moduleColor.isTransparent()
        ? BiztortionTheme::controlAccent
        : moduleColor;

    for (const auto& group : parameterGroups)
    {
        const auto plate = group.toFloat();
        g.setColour(accent.withAlpha(0.055f));
        g.fillRoundedRectangle(plate, 6.0f);
        g.setColour(accent.withAlpha(0.32f));
        g.drawRoundedRectangle(plate, 6.0f, 1.0f);
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