/*
  ==============================================================================

    OscilloscopeModule.cpp
    Created: 9 Sep 2021 9:24:50pm
    Author:  gabri

  ==============================================================================
*/

//==============================================================================

/* OscilloscopeModule DSP */

//==============================================================================

#include "OscilloscopeModule.h"
#include "../PluginProcessor.h"

OscilloscopeModuleDSP::OscilloscopeModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
    oscilloscope.clear();
}

drow::AudioOscilloscope* OscilloscopeModuleDSP::getOscilloscope()
{
    return &oscilloscope;
}

OscilloscopeSettings OscilloscopeModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition)
{
    OscilloscopeSettings settings;
    settings.hZoom = apvts.getRawParameterValue("Oscilloscope H Zoom " + std::to_string(chainPosition))->load();
    settings.vZoom = apvts.getRawParameterValue("Oscilloscope V Zoom " + std::to_string(chainPosition))->load();
    settings.bypassed = apvts.getRawParameterValue("Oscilloscope Bypassed " + std::to_string(chainPosition))->load() > 0.5f;

    return settings;
}

void OscilloscopeModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for (int i = 1; i < 9; ++i) {
        layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Oscilloscope H Zoom " + std::to_string(i), 
            "Oscilloscope H Zoom " + std::to_string(i), 
            juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 0.f, "Oscilloscope " + std::to_string(i))));
        layout.add(std::move(std::make_unique<juce::AudioParameterFloat>("Oscilloscope V Zoom " + std::to_string(i), 
            "Oscilloscope V Zoom " + std::to_string(i), 
            juce::NormalisableRange<float>(0.f, 1.f, 0.01f), 1.f, "Oscilloscope " + std::to_string(i))));
        // bypass button
        layout.add(std::make_unique<AudioParameterBool>("Oscilloscope Bypassed " + std::to_string(i), "Oscilloscope Bypassed " + std::to_string(i), 
            false, "Oscilloscope " + std::to_string(i)));
    }
}

void OscilloscopeModuleDSP::setModuleType()
{
    moduleType = ModuleType::Oscilloscope;
}

void OscilloscopeModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, getChainPosition());
    bypassed = settings.bypassed;
    oscilloscope.setHorizontalZoom(settings.hZoom);
    oscilloscope.setVerticalZoom(settings.vZoom);
}

void OscilloscopeModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    oscilloscope.clear();
    updateDSPState(sampleRate);
}

void OscilloscopeModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    updateDSPState(sampleRate);
    if (!bypassed) {
        oscilloscope.processBlock(buffer.getReadPointer(0), buffer.getNumSamples());
    }
}

//==============================================================================

/* OscilloscopeModule GUI */

//==============================================================================

OscilloscopeModuleGUI::OscilloscopeModuleGUI(BiztortionAudioProcessor& p, drow::AudioOscilloscope* _oscilloscope, unsigned int chainPosition)
    : GUIModule(), audioProcessor(p), oscilloscope(_oscilloscope),
    hZoomSlider(*audioProcessor.apvts.getParameter("Oscilloscope H Zoom " + std::to_string(chainPosition)), ""),
    vZoomSlider(*audioProcessor.apvts.getParameter("Oscilloscope V Zoom " + std::to_string(chainPosition)), ""),
    hZoomSliderAttachment(audioProcessor.apvts, "Oscilloscope H Zoom " + std::to_string(chainPosition), hZoomSlider),
    vZoomSliderAttachment(audioProcessor.apvts, "Oscilloscope V Zoom " + std::to_string(chainPosition), vZoomSlider),
    bypassButtonAttachment(audioProcessor.apvts, "Oscilloscope Bypassed " + std::to_string(chainPosition), bypassButton)
{
    // title setup
    title.setText("Oscilloscope", juce::dontSendNotification);
    title.setFont(24);

    // labels

    hZoomLabel.setText("H Zoom", juce::dontSendNotification);
    hZoomLabel.setFont(10);
    vZoomLabel.setText("V Zoom", juce::dontSendNotification);
    vZoomLabel.setFont(10);

    hZoomSlider.labels.add({ 0.f, "0" });
    hZoomSlider.labels.add({ 1.f, "1" });
    vZoomSlider.labels.add({ 0.f, "0" });
    vZoomSlider.labels.add({ 1.f, "1" });

    // bypass button
    bypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<OscilloscopeModuleGUI>(this);
    bypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->bypassButton.getToggleState();

            comp->hZoomSlider.setEnabled(!bypassed);
            comp->vZoomSlider.setEnabled(!bypassed);
            comp->freezeButton.setEnabled(!bypassed);
        }
    };

    // freeze button
    freezeButton.setClickingTogglesState(true);
    freezeButton.setToggleState(false, juce::dontSendNotification);

    freezeLnf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    freezeLnf.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    freezeLnf.setColour(juce::TextButton::buttonOnColourId, juce::Colours::black);
    freezeLnf.setColour(juce::TextButton::textColourOnId, juce::Colours::lightgreen);

    freezeButton.setLookAndFeel(&freezeLnf);

    freezeButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto freeze = comp->freezeButton.getToggleState();
            if (freeze) {
                comp->oscilloscope->stopTimer();
            }
            else {
                comp->oscilloscope->startTimerHz(59);
            }
            
        }
    };

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    oscilloscope->startTimerHz(60);
}

OscilloscopeModuleGUI::~OscilloscopeModuleGUI()
{
    oscilloscope->stopTimer();
    oscilloscope = nullptr;
    freezeButton.setLookAndFeel(nullptr);
    bypassButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> OscilloscopeModuleGUI::getComps()
{
    return {
        oscilloscope,
        &title,
        &hZoomLabel,
        &vZoomLabel,
        &hZoomSlider,
        &vZoomSlider,
        &freezeButton,
        &bypassButton
    };
}

void OscilloscopeModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void OscilloscopeModuleGUI::resized()
{
    auto oscilloscopeArea = getContentRenderArea();

    // bypass
    auto temp = oscilloscopeArea;
    auto bypassButtonArea = temp.removeFromTop(25);

    bypassButtonArea.setWidth(35.f);
    bypassButtonArea.setX(145.f);
    bypassButtonArea.setY(20.f);

    bypassButton.setBounds(bypassButtonArea);

    auto titleAndBypassArea = oscilloscopeArea.removeFromTop(30);

    auto graphArea = oscilloscopeArea.removeFromTop(oscilloscopeArea.getHeight() * (2.f / 3.f));
    graphArea.reduce(10, 10);

    auto freezeArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * (1.f / 3.f));
    juce::Rectangle<int> freezeCorrectArea = freezeArea;
    freezeCorrectArea.reduce(50.f, 34.f);
    freezeCorrectArea.setCentre(freezeArea.getCentre());

    auto hZoomArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * (1.f / 2.f));
    auto hZoomLabelArea = hZoomArea.removeFromTop(12);

    auto vZoomArea = oscilloscopeArea;
    auto vZoomLabelArea = vZoomArea.removeFromTop(12);

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centred);

    hZoomLabel.setBounds(hZoomLabelArea);
    hZoomLabel.setJustificationType(juce::Justification::centred);
    vZoomLabel.setBounds(vZoomLabelArea);
    vZoomLabel.setJustificationType(juce::Justification::centred);

    oscilloscope->setBounds(graphArea);
    hZoomSlider.setBounds(hZoomArea);
    vZoomSlider.setBounds(vZoomArea);
    freezeButton.setBounds(freezeCorrectArea);
}
