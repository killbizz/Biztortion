/*
  ==============================================================================

    GUIModule.h

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

#pragma once

#include <JuceHeader.h>

#include <random>
#include <vector>

#include "../Component/HelpComponent.h"
#include "../Shared/GUIStuff.h"

class GUIModule : public juce::Component, public juce::Timer {
public:
    GUIModule();
    virtual ~GUIModule();
    void handleParamCompsEnablement(bool bypass);

    // basic GUImodule painting (a single GUImodule can override this method to generate a more complex paint method)
    void paint(juce::Graphics& g) override;

    void timerCallback() override;

    // facility for addAndMakeVisible function
    virtual std::vector<juce::Component*> getAllComps() = 0;
    // facility for handleParamCompsEnablement function (enablement or disablement of parameters based on bypass param)
    virtual std::vector<juce::Component*> getParamComps() = 0;
    // update the parameters of this GUIModule using the parameter values from values array
    virtual void updateParameters(const juce::Array<juce::var>& values) = 0;
    // reset the parameters of this GUIModule to default values
    virtual void resetParameters(unsigned int parameterNumber) = 0;
    // get parameter values of this GUIModule (vars in the array can be bool or double values)
    virtual juce::Array<juce::var> getParamValues() = 0;

    // size of the module header buttons (bypass, help): keeps every module header coherent
    static constexpr int headerButtonSize = 28;
    // horizontal gap kept around the help button, so it never crowds a module's own controls
    static constexpr int headerButtonGap = 8;
    // width a module's resized() must leave free on the right of its header for the help button
    static constexpr int headerHelpButtonArea = headerButtonSize + headerButtonGap * 2;

protected:
    juce::Rectangle<int> getContentRenderArea();
    juce::Rectangle<int> getContainerArea();
    virtual void drawContainer(juce::Graphics& g);
    void applyModuleAccentToSliders();
    void setModuleHelp(juce::String title, juce::String description);

    // Module-specific parameter clusters (e.g. rise/fall of the slew limiter). They are painted
    // as a subtle tinted plate behind the knobs so they read apart from the shared parameters
    // (drive, mix, fx distribution, fx bias, symmetry, dc filter) without adding visual noise.
    // Call from resized(), after the knob areas have been computed.
    void clearParameterGroups();
    void addParameterGroup(juce::Rectangle<int> area);

    juce::Colour moduleColor;
    unsigned int brightnessCounter;
    int addingFactor = 1;
    juce::NormalisableRange<float> remapper;

  private:
    void showModuleHelpWindow();
    void drawParameterGroups(juce::Graphics& g);

    juce::TextButton moduleHelpButton { "?" };
    juce::String moduleHelpTitle;
    juce::String moduleHelpDescription;

    GlyphButtonLookAndFeel moduleHelpButtonLookAndFeel;

    // rendered with the same look as the plugin's "About Biztortion" window
    ModuleLookAndFeel helpWindowLookAndFeel;
    std::unique_ptr<HelpComponent> moduleHelpComponent;
    std::unique_ptr<juce::AlertWindow> moduleHelpWindow;

    std::vector<juce::Rectangle<int>> parameterGroups;
};