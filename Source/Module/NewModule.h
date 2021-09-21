/*
  ==============================================================================

    NewModule.h
    Created: 2 Sep 2021 10:11:11am
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "GUIModule.h"
#include "DSPModule.h"
#include "../Component/GUIStuff.h"
class BiztortionAudioProcessor;
class BiztortionAudioProcessorEditor;

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
    void setupDeleteModuleColours(juce::LookAndFeel& laf);
    void setupCurrentModuleActivatorColours(juce::LookAndFeel& laf);

    void addModuleToGUI(GUIModule* module);
    void addModuleToDSPmodules(DSPModule* module);
    void newModuleSetup(ModuleType type);

private:

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;
    BiztortionAudioProcessorEditor& editor;
    unsigned int chainPosition;
    juce::Label chainPositionLabel;

    // newModule
    juce::ComboBox newModuleSelector;
    ModuleLookAndFeel newModuleLookAndFeel;
    juce::TextButton newModule{ "+" };

    // deleteModule
    ModuleLookAndFeel deleteModuleLookAndFeel;
    juce::TextButton deleteModule{ "x" };

    // currentModuleActivator
    ModuleLookAndFeel currentModuleActivatorLookAndFeel;
    juce::TextButton currentModuleActivator;

    ModuleType moduleType = ModuleType::Uninstantiated;

    GUIModule* createGUIModule(ModuleType type);
};