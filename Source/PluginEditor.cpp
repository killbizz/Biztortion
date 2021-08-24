/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BiztortionAudioProcessorEditor::BiztortionAudioProcessorEditor (BiztortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), 
    responseCurveComponent(p), analyzerComponent(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    setSize (700, 600);
    setResizable(true, true);
    setResizeLimits(600, 500, 900, 800);
}

BiztortionAudioProcessorEditor::~BiztortionAudioProcessorEditor()
{
}

//==============================================================================
void BiztortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    
}

void BiztortionAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();

    // filters
    auto filtersArea = bounds.removeFromTop(bounds.getHeight() * (2.f / 3.f));
    auto responseCurveArea = filtersArea.removeFromTop(filtersArea.getHeight() * (1.f / 2.f));
    auto lowCutArea = filtersArea.removeFromLeft(filtersArea.getWidth() * (1.f / 3.f));
    auto highCutArea = filtersArea.removeFromRight(filtersArea.getWidth() * (1.f / 2.f));

    responseCurveComponent.setBounds(responseCurveArea);
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * (1.f / 2.f)));
    highCutSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * (1.f / 2.f)));
    peakFreqSlider.setBounds(filtersArea.removeFromTop(filtersArea.getHeight() * 0.33));
    peakGainSlider.setBounds(filtersArea.removeFromTop(filtersArea.getHeight() * 0.5));
    peakQualitySlider.setBounds(filtersArea);
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutSlopeSlider.setBounds(highCutArea);

    // analyzer
    auto analyzerArea = bounds.removeFromRight(bounds.getWidth() * (1.f / 2.f));

    analyzerComponent.setBounds(analyzerArea);

    // oscilloscope
    audioProcessor.oscilloscope.setBounds(bounds);
}

std::vector<juce::Component*> BiztortionAudioProcessorEditor::getComps()
{
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,
        &analyzerComponent,
        &(audioProcessor.oscilloscope)
    };
}
