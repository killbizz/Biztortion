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
    responseCurveComponent(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    setSize (700, 600);
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

    auto responseCurveArea = bounds.removeFromTop(bounds.getHeight() * (1.f / 3.f));

    responseCurveComponent.setBounds(responseCurveArea);

    // auto oscilloscopeArea = bounds.removeFromTop(getHeight() / 2);
    audioProcessor.oscilloscope.setBounds(bounds);
}

std::vector<juce::Component*> BiztortionAudioProcessorEditor::getComps()
{
    return {
        &responseCurveComponent,
        &(audioProcessor.oscilloscope)
    };
}
