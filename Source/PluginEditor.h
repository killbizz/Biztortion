/*
  ==============================================================================

    PluginEditor.h

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

#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "Component/FFTAnalyzerComponent.h"
#include "Component/TransferFunctionGraphComponent.h"
#include "Component/HelpComponent.h"
#include "Shared/GUIStuff.h"
#include "Module/ChainModule.h"
#include "Module/WelcomeModule.h"

//==============================================================================
/** EDITOR
*/
class BiztortionAudioProcessorEditor  : public juce::AudioProcessorEditor, public DragAndDropContainer
{
public:
    BiztortionAudioProcessorEditor (juce::AudioProcessor& ap, PluginState& ps);
    ~BiztortionAudioProcessorEditor() override;

    /*void editorSetup();*/
    /*GUIModule* createGUIModule(ModuleType type, unsigned int chainPosition);*/
    /*void updateCurrentGUIModule(GUIModule* module);*/

    void paint (juce::Graphics&) override;
    void resized() override;

    /*std::unique_ptr<GUIModule> currentGUIModule;
    std::vector<std::unique_ptr<ChainModuleGUI>> newModules;*/

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    struct AsyncAlertBoxResult
    {
        void operator() (int result) const noexcept
        {
            auto& aw = *editor.asyncAlertWindow;

            aw.exitModalState(result);
            aw.setVisible(false);
        }

        BiztortionAudioProcessorEditor& editor;
    };

    juce::TooltipWindow tooltipWindow;

    GUIState guiState;

    /*std::unique_ptr<GUIModule> inputMeter;
    std::unique_ptr<GUIModule> outputMeter;*/

    juce::HyperlinkButton helpButton;
    juce::HyperlinkButton githubLink;

    std::unique_ptr<AlertWindow> asyncAlertWindow;
    HelpComponent helpComponent;

    ModuleLookAndFeel laf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BiztortionAudioProcessorEditor)
};