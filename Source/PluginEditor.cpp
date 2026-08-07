/*
  ==============================================================================

    PluginEditor.cpp

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

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

#include "Module/ChainModule/ChainModule.h"

//==============================================================================
BiztortionAudioProcessorEditor::BiztortionAudioProcessorEditor (juce::AudioProcessor& ap, PluginState& ps)
    : AudioProcessorEditor (&ap), guiState(*this, ps), githubLink("About", {"https://github.com/killbizz"}), title("Biztortion", *this)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    setLookAndFeel(&laf);

    title.setFont(juce::Font("Segoe UI", 26, juce::Font::bold));
    title.setColour(juce::Label::ColourIds::textColourId, BiztortionTheme::text);
    title.setTooltip("Show the Homepage");
    addAndMakeVisible(title);

    tooltipWindow.setLookAndFeel(&laf);

    helpButton.setFont(juce::Font("Segoe UI", 14, juce::Font::plain), true);
    helpButton.setButtonText("Help");
    helpButton.setTooltip("Show some useful informations about this software");
    helpButton.setColour(juce::HyperlinkButton::textColourId, juce::Colours::white);
    addAndMakeVisible(helpButton);

    helpButton.onClick = [this] {
        helpComponent.setBounds(0, 0, 450, 250);
        asyncAlertWindow = std::make_unique<AlertWindow>("Welcome to Biztortion!",
            "",
            MessageBoxIconType::NoIcon);
        asyncAlertWindow->setLookAndFeel(&laf);
        asyncAlertWindow->addButton("Got it!", 1, KeyPress(KeyPress::returnKey, 0, 0));
        asyncAlertWindow->addCustomComponent(&helpComponent);

        // setting colors
        asyncAlertWindow->setColour(juce::AlertWindow::backgroundColourId, juce::Colours::darkgrey.withAlpha(0.75f));
        asyncAlertWindow->setColour(juce::AlertWindow::textColourId, juce::Colours::white);
        asyncAlertWindow->setColour(juce::AlertWindow::outlineColourId, juce::Colours::black);

        asyncAlertWindow->enterModalState(true, ModalCallbackFunction::create(AsyncAlertBoxResult{ *this }));
    };

    githubLink.setFont(juce::Font("Segoe UI", 14, juce::Font::plain), true);
    githubLink.setColour(juce::HyperlinkButton::textColourId, BiztortionTheme::textMuted);
    addAndMakeVisible(githubLink);

    setSize(1120, 720);
    setResizable(true, true);
    setResizeLimits(900, 577, 1680, 1080);
}

BiztortionAudioProcessorEditor::~BiztortionAudioProcessorEditor()
{
    tooltipWindow.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

//==============================================================================
void BiztortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;

    g.fillAll(BiztortionTheme::canvas);

    auto header = getLocalBounds().removeFromTop(52).toFloat();
    g.setColour(BiztortionTheme::surface);
    g.fillRect(header);
    g.setColour(BiztortionTheme::border);
    g.drawLine(0.0f, header.getBottom(), (float)getWidth(), header.getBottom(), 1.0f);
    
}

void BiztortionAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds().reduced(10);
    auto header = bounds.removeFromTop(42);
    auto titleBounds = header.withSizeKeepingCentre(260, header.getHeight());

    title.setBounds(titleBounds);
    title.setJustificationType(juce::Justification::centred);

    helpButton.setBounds(header.removeFromLeft(90));
    githubLink.setBounds(header.removeFromRight(90));

    bounds.removeFromTop(8);
    auto chainArea = bounds.removeFromBottom(170);
    bounds.removeFromBottom(8);
    auto inputMeterArea = bounds.removeFromLeft(148);
    bounds.removeFromLeft(8);
    auto outputMeterArea = bounds.removeFromRight(148);
    bounds.removeFromRight(8);
    auto currentGUImoduleArea = bounds;

    // chopping the chainArea in 8 equal rectangles for 8 NewModules
    std::vector<juce::Rectangle<int>> chainAreas;
    const auto gap = 6;
    const auto width = (chainArea.getWidth() - gap * 7) / 8;
    for (int i = 0; i < 8; ++i) {
        chainAreas.push_back(chainArea.removeFromLeft(width));
        if (i < 7)
            chainArea.removeFromLeft(gap);
    }

    guiState.inputMeter->setBounds(inputMeterArea);
    guiState.outputMeter->setBounds(outputMeterArea);
    if(guiState.currentGUIModule)    guiState.currentGUIModule->setBounds(currentGUImoduleArea);
    auto it = guiState.chainModules.begin();
    for (int i = 0; i < 8; ++i) {
        (*it)->setBounds(chainAreas[i]);
        ++it;
    }
}