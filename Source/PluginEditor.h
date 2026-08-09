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

      class HeaderIconButton : public juce::Button
      {
      public:
        enum class Icon { help, github };

        explicit HeaderIconButton(Icon iconToDraw)
          : juce::Button(iconToDraw == Icon::help ? "Help" : "GitHub"), icon(iconToDraw) {}

        void paintButton(juce::Graphics& g, bool highlighted, bool down) override
        {
          auto bounds = getLocalBounds().toFloat().reduced(1.0f);
          g.setColour((highlighted ? BiztortionTheme::surfaceRaised.brighter(0.12f)
                       : BiztortionTheme::surfaceRaised)
                  .withMultipliedBrightness(down ? 0.85f : 1.0f));
          g.fillRoundedRectangle(bounds, 6.0f);
          g.setColour(highlighted ? BiztortionTheme::controlAccent : BiztortionTheme::border);
          g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
          g.setColour(BiztortionTheme::text);

          if (icon == Icon::help)
          {
            g.setFont(juce::Font("Segoe UI", 18.0f, juce::Font::bold));
            g.drawText("?", getLocalBounds(), juce::Justification::centred);
            return;
          }

          const auto centre = bounds.getCentre();
          juce::Path branch;
          branch.startNewSubPath(centre.x - 5.0f, centre.y - 7.0f);
          branch.lineTo(centre.x - 5.0f, centre.y + 7.0f);
          branch.startNewSubPath(centre.x - 5.0f, centre.y);
          branch.cubicTo(centre.x + 5.0f, centre.y, centre.x + 5.0f, centre.y - 4.0f,
                   centre.x + 5.0f, centre.y - 7.0f);
          g.strokePath(branch, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved));
          for (auto point : { juce::Point<float>(centre.x - 5.0f, centre.y - 7.0f),
                    juce::Point<float>(centre.x - 5.0f, centre.y + 7.0f),
                    juce::Point<float>(centre.x + 5.0f, centre.y - 7.0f) })
            g.fillEllipse(point.x - 2.2f, point.y - 2.2f, 4.4f, 4.4f);
        }

      private:
        Icon icon;
      };

    TitleLabel title;

    juce::TooltipWindow tooltipWindow;

    HeaderIconButton helpButton { HeaderIconButton::Icon::help };
    HeaderIconButton githubLink { HeaderIconButton::Icon::github };

    std::unique_ptr<AlertWindow> asyncAlertWindow;
    HelpComponent helpComponent;

    ModuleLookAndFeel laf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BiztortionAudioProcessorEditor)
};