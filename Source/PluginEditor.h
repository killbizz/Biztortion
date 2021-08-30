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

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;

    std::vector<std::unique_ptr<Component>> modules;

    juce::ToggleButton newModule;
    juce::ComboBox newModuleSelector;

    //FilterModuleGUI filterModuleGUI;
    // waveshaperModule
    TransferFunctionGraphComponent transferFunctionGraph;
    // juce::LookAndFeel_V4 lookAndFeel1, lookAndFeel2, lookAndFeel3;
    /*juce::Label waveshaperDriveLabel,
        waveshaperMixLabel,
        tanhAmpLabel,
        tanhSlopeLabel,
        sineAmpLabel,
        sineFreqLabel;*/
    RotarySliderWithLabels waveshaperDriveSlider,
        waveshaperMixSlider,
        tanhAmpSlider,
        tanhSlopeSlider,
        sineAmpSlider,
        sineFreqSlider;
    Attachment waveshaperDriveSliderAttachment,
        waveshaperMixSliderAttachment,
        tanhAmpSliderAttachment,
        tanhSlopeSliderAttachment,
        sineAmpSliderAttachment,
        sineFreqSliderAttachment;

    // fft analyzer
    FFTAnalyzerComponent analyzerComponent;

    std::vector<juce::Component*> getComps();
    void updateGUI();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BiztortionAudioProcessorEditor)
};