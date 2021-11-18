/*
  ==============================================================================

    NewModuleModule.h

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

#include "GUIModule.h"
#include "DSPModule.h"
#include "../Shared/GUIStuff.h"
class BiztortionAudioProcessor;
class BiztortionAudioProcessorEditor;

class BizComboBox : public juce::ComboBox {
public:
    BizComboBox(juce::TextButton* nm) {
        newModule = nm;
    }

    void focusLost(juce::Component::FocusChangeType ct) override {
        hidePopup();
        setVisible(false);
        juce::ComboBox::focusLost(ct);
        newModule->setToggleState(false, juce::NotificationType::dontSendNotification);
    }
private:
    juce::TextButton* newModule;
};

//==============================================================================

/* NewModule GUI */

//==============================================================================

class NewModuleGUI : public GUIModule {
public:

    NewModuleGUI(BiztortionAudioProcessor& p, BiztortionAudioProcessorEditor& e, unsigned int _chainPosition);
    ~NewModuleGUI();

    unsigned int getChainPosition();
    void setChainPosition(unsigned int cp);

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::vector<juce::Component*> getComps() override;
    void setupNewModuleColours(juce::LookAndFeel& laf);
    void setupNewModuleSelectorColours(juce::LookAndFeel& laf);
    void setupDeleteModuleColours(juce::LookAndFeel& laf);
    void setupCurrentModuleActivatorColours(juce::LookAndFeel& laf);

    void addModuleToGUI(GUIModule* module);
    void newModuleSetup(ModuleType type);
    void resetButtonsColors();

    // newModule
    ModuleLookAndFeel newModuleLookAndFeel;
    juce::TextButton newModule{ "+" };
    ModuleLookAndFeel newModuleSelectorLookAndFeel;
    BizComboBox newModuleSelector{ &newModule };

    // deleteModule
    ModuleLookAndFeel deleteModuleLookAndFeel;
    juce::TextButton deleteModule{ "X" };

    // currentModuleActivator
    ModuleLookAndFeel currentModuleActivatorLookAndFeel;
    juce::TextButton currentModuleActivator;

private:

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;
    BiztortionAudioProcessorEditor& editor;
    unsigned int chainPosition;
    juce::Label chainPositionLabel;

    ModuleType moduleType = ModuleType::Uninstantiated;

    // UNUSED only by the 8° module
    std::unique_ptr<juce::Drawable> rightCable;

    GUIModule* createGUIModule(ModuleType type);
    juce::AffineTransform getTransform();
    std::unique_ptr<juce::Drawable> getRightCable(unsigned int chainPosition);
};