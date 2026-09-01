/*
  ==============================================================================

    ChainModule.h

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

#include "../../Shared/GUIStuff.h"
#include "../../Shared/GUIState.h"
#include "../../Shared/PluginState.h"
#include "../../Module/WelcomeModule/WelcomeModule.h"
#include "../../Shared/ModuleGenerator.h"

//==============================================================================

/* Audio cable linking two chain slots */

//==============================================================================

// Painted instead of tinting the whole whiteCable.svg drawable, so the parts can animate
// independently: the plugs and the cable body stay a stable white while only the inner
// core pulses to suggest the signal flowing left to right.
class CableComponent : public juce::Component
{
public:
    CableComponent();

    // 0..1 animation position of the inner core
    void setAnimationPhase(float newPhase);

    void paint(juce::Graphics& g) override;

private:
    float phase = 0.0f;

    // geometry of the original whiteCable.svg artwork, kept so the cable keeps its shape
    static constexpr float artworkWidth = 431.65f;
    static constexpr float artworkHeight = 169.68f;
};

//==============================================================================

/* ChainModule GUI */

//==============================================================================

class ChainModuleGUI : public GUIModule, public DragAndDropTarget {
public:

    ChainModuleGUI(PluginState& ps, GUIState& gs, unsigned int _chainPosition);
    ~ChainModuleGUI();

    unsigned int getChainPosition();
    void setChainPosition(unsigned int cp);
    ModuleType getModuleType();
    void setModuleType(ModuleType mt);

    void addNewModule(ModuleType type);
    void deleteTheCurrentChainModule();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;

    std::vector<juce::Component*> getAllComps() override;
    std::vector<juce::Component*> getParamComps() override;

    // useless methods because this is not a module usable in the processing chain
    virtual void updateParameters(const juce::Array<juce::var>&) override {};
    virtual void resetParameters(unsigned int) override {};
    virtual juce::Array<juce::var> getParamValues() override { return juce::Array<juce::var>(); };

    void setupNewModuleColours(juce::LookAndFeel& laf);
    void setupNewModuleSelectorColours(juce::LookAndFeel& laf);
    void setupDeleteModuleColours(juce::LookAndFeel& laf);
    void setupCurrentModuleActivatorColours(juce::LookAndFeel& laf);

    void addModuleToGUI(GUIModule* module);
    // shows this slot's module in the main view (what clicking the slot button does)
    void openInMainView();
    void setup(ModuleType type);

    // These methods implement the DragAndDropTarget interface, and allow our component
    // to accept drag-and-drop of objects from other JUCE components..
    bool isInterestedInDragSource(const SourceDetails& /*dragSourceDetails*/) override;
    void itemDragEnter(const SourceDetails& /*dragSourceDetails*/) override;
    void itemDragMove(const SourceDetails& /*dragSourceDetails*/) override;
    void itemDragExit(const SourceDetails& /*dragSourceDetails*/) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    void mouseDrag(const MouseEvent& event) override;

    //==============================================================================

    // newModule
    ModuleLookAndFeel newModuleLookAndFeel;
    juce::TextButton newModule{ "+" };
    ModuleLookAndFeel newModuleSelectorLookAndFeel;

    // deleteModule
    ModuleLookAndFeel deleteModuleLookAndFeel;
    juce::DrawableButton deleteModule{"Delete", juce::DrawableButton::ButtonStyle::ImageFitted };

    ButtonsLookAndFeel bypassLookAndFeel;
    PowerButton bypassButton;

    // currentModuleActivator
    ModuleLookAndFeel currentModuleActivatorLookAndFeel;
    BizTextButton currentModuleActivator;

    BizLabel chainPositionLabel;
    std::unique_ptr<juce::Drawable> dragIcon;

    // inline editor shown over the slot button while renaming the module
    juce::Label renameLabel;

private:

    PluginState& pluginState;
    GUIState& guiState;

    unsigned int chainPosition;

    ModuleType moduleType = ModuleType::Uninstantiated;
    ModuleGenerator moduleGenerator;

    // nullptr only in the 8° module
    std::unique_ptr<CableComponent> rightCable;
    juce::Rectangle<int> getCableBounds() const;

    bool somethingIsBeingDraggedOver = false;

    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttachment> bypassButtonAttachment;

    void showNewModuleMenu();
    juce::String getBypassParameterID() const;

    // module renaming
    void beginRenaming();
    void commitRename();
    void updateModuleNameDisplay();
    unsigned int getParameterNumber() const;

protected:

    void drawContainer(juce::Graphics& g) override;
};