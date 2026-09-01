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
    : AudioProcessorEditor (&ap), guiState(*this, ps), title("Biztortion", *this)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    setLookAndFeel(&laf);

    title.setFont(juce::Font("Segoe UI", 26, juce::Font::bold));
    title.setColour(juce::Label::ColourIds::textColourId, BiztortionTheme::text);
    title.setTooltip("Show the Homepage");
    addAndMakeVisible(title);

    tooltipWindow.setLookAndFeel(&laf);

    helpButton.setTooltip("About Biztortion");
    addAndMakeVisible(helpButton);

    helpButton.onClick = [this] {
        constexpr int helpWidth = 460;
        helpComponent.setBounds(0, 0, helpWidth,
            helpComponent.getPreferredHeightForWidth(helpWidth));
        asyncAlertWindow = std::make_unique<AlertWindow>("Biztortion",
            "",
            MessageBoxIconType::NoIcon);
        asyncAlertWindow->setLookAndFeel(&laf);
        asyncAlertWindow->addButton("Got it!", 1, KeyPress(KeyPress::returnKey, 0, 0));
        asyncAlertWindow->addCustomComponent(&helpComponent);

        // setting colors
        asyncAlertWindow->setColour(juce::AlertWindow::backgroundColourId, BiztortionTheme::surface);
        asyncAlertWindow->setColour(juce::AlertWindow::textColourId, BiztortionTheme::text);
        asyncAlertWindow->setColour(juce::AlertWindow::outlineColourId, BiztortionTheme::border);

        asyncAlertWindow->enterModalState(true, ModalCallbackFunction::create(AsyncAlertBoxResult{ *this }));
    };

    legendButton.setTooltip("Show the module colour legend");
    addAndMakeVisible(legendButton);

    legendButton.onClick = [this] {
        auto legend = std::make_unique<ModuleLegendComponent>();
        // opens right below the button
        juce::CallOutBox::launchAsynchronously(std::move(legend),
            legendButton.getScreenBounds(), nullptr);
    };

    githubLink.setTooltip("Open the Biztortion GitHub repository");
    githubLink.onClick = [] { juce::URL("https://github.com/killbizz/Biztortion").launchInDefaultBrowser(); };
    addAndMakeVisible(githubLink);

    setSize(1120, 720);
    setResizable(true, true);
    // limits chosen to match the 1120x720 ratio exactly, so the constrainer never has to
    // fight them while dragging a corner
    setResizeLimits(900, 579, 1680, 1080);

    // resize proportionally only: stretching one axis alone wrecks the module layouts
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio(1120.0 / 720.0);

    // Done last: reopening the editor comes back to the module the user was looking at, and
    // restoring lays out the editor, so every child component has to exist first.
    guiState.restoreLastViewedModule();
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
    auto header = bounds.removeFromTop(32);
    auto titleBounds = header.withSizeKeepingCentre(260, header.getHeight());

    title.setBounds(titleBounds);
    title.setJustificationType(juce::Justification::centred);

    helpButton.setBounds(header.removeFromLeft(32));
    header.removeFromLeft(8);
    legendButton.setBounds(header.removeFromLeft(32));
    githubLink.setBounds(header.removeFromRight(32));

    bounds.removeFromTop(20);
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