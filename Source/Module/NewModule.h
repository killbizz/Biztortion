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

// classes for drag-and-drop logic

class BizDrawable : public juce::DrawableImage {
public:
    BizDrawable() = default;
    BizDrawable(const juce::DrawableImage& d) : juce::DrawableImage(d){}
    void mouseDrag(const MouseEvent& event) override;
};

class BizTextButton : public juce::TextButton {
public:
    BizTextButton() = default;
    void mouseDrag(const MouseEvent & event) override;
};

class BizLabel : public juce::Label {
public:
    void mouseDrag(const MouseEvent& event) override;
};
//==============================================================================

/* NewModule GUI */

//==============================================================================

class NewModuleGUI : public GUIModule, public DragAndDropTarget {
public:

    NewModuleGUI(BiztortionAudioProcessor& p, BiztortionAudioProcessorEditor& e, unsigned int _chainPosition);
    ~NewModuleGUI();

    unsigned int getChainPosition();
    void setChainPosition(unsigned int cp);
    ModuleType getModuleType();
    void setModuleType(ModuleType mt);

    void addNewModule(ModuleType type);
    void deleteTheCurrentNewModule();

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::vector<juce::Component*> getAllComps() override;
    std::vector<juce::Component*> getParamComps() override;
    // useless methods because (at this moment) this is not a module usable in the processing chain
    virtual void updateParameters(const juce::Array<juce::var>&) override {};
    virtual void resetParameters(unsigned int) override {};
    virtual juce::Array<juce::var> getParamValues() override { return juce::Array<juce::var>(); };

    void setupNewModuleColours(juce::LookAndFeel& laf);
    void setupNewModuleSelectorColours(juce::LookAndFeel& laf);
    void setupDeleteModuleColours(juce::LookAndFeel& laf);
    void setupCurrentModuleActivatorColours(juce::LookAndFeel& laf);

    void addModuleToGUI(GUIModule* module);
    void newModuleSetup(ModuleType type);

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
    BizTextButton currentModuleActivator;

    // These methods implement the DragAndDropTarget interface, and allow our component
    // to accept drag-and-drop of objects from other JUCE components..
    bool isInterestedInDragSource(const SourceDetails& /*dragSourceDetails*/) override;
    void itemDragEnter(const SourceDetails& /*dragSourceDetails*/) override;
    void itemDragMove(const SourceDetails& /*dragSourceDetails*/) override;
    void itemDragExit(const SourceDetails& /*dragSourceDetails*/) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    void mouseDrag(const MouseEvent& event) override;

private:

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;
    BiztortionAudioProcessorEditor& editor;
    unsigned int chainPosition;
    BizLabel chainPositionLabel;

    ModuleType moduleType = ModuleType::Uninstantiated;

    // UNUSED only by the 8� module
    std::unique_ptr<BizDrawable> rightCable;
    juce::AffineTransform getTransform();
    std::unique_ptr<BizDrawable> getRightCable(unsigned int chainPosition);

    bool somethingIsBeingDraggedOver = false;
};