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

#include "Component/FFTAnalyzerComponent.h"
#include "Component/TransferFunctionGraphComponent.h"
#include "Component/HelpComponent.h"
#include "Shared/GUIStuff.h"
#include "Shared/GUIState.h"
#include "Module/WelcomeModule/WelcomeModule.h"

class BiztortionAudioProcessorEditor : public juce::AudioProcessorEditor, public DragAndDropContainer
{
public:
    BiztortionAudioProcessorEditor(juce::AudioProcessor& ap, PluginState& ps);
    ~BiztortionAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    //==============================================================================

    GUIState guiState;

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

    class TitleLabel : public Label
    {
    public:

        TitleLabel(juce::String title, BiztortionAudioProcessorEditor& e) : juce::Label(title, title), editor(e) {
            setMouseCursor(MouseCursor::PointingHandCursor);
        }
        void mouseDown(const MouseEvent& event) override {
            editor.guiState.updateCurrentGUIModule(new WelcomeModuleGUI());
        }

    private:
        BiztortionAudioProcessorEditor& editor;
    };

    TitleLabel title;

    juce::TooltipWindow tooltipWindow;

    juce::HyperlinkButton helpButton;
    juce::HyperlinkButton githubLink;

    std::unique_ptr<AlertWindow> asyncAlertWindow;
    HelpComponent helpComponent;

    ModuleLookAndFeel laf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BiztortionAudioProcessorEditor)
};