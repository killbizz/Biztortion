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
#include "Component/GUIStuff.h"
#include "Module/FilterModule.h"
#include "Module/WaveshaperModule.h"
#include "Module/NewModule.h"
#include "Module/WelcomeModule.h"

//==============================================================================
/** EDITOR
*/
class BiztortionAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    BiztortionAudioProcessorEditor (BiztortionAudioProcessor&);
    ~BiztortionAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void updateGUI();

    // std::vector<std::unique_ptr<GUIModule>> GUImodules;
    std::unique_ptr<GUIModule> inputMeter;
    std::unique_ptr<GUIModule> outputMeter;
    std::unique_ptr<GUIModule> currentGUIModule;
    std::vector<std::unique_ptr<NewModuleGUI>> newModules;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;

    juce::HyperlinkButton githubLink;

    //std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BiztortionAudioProcessorEditor)
};