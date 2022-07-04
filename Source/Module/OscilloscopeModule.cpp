/*
  ==============================================================================

    OscilloscopeModule.cpp

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

//==============================================================================

/* OscilloscopeModule DSP */

//==============================================================================

#include "OscilloscopeModule.h"

OscilloscopeModuleDSP::OscilloscopeModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
    leftOscilloscope.clear();
    rightOscilloscope.clear();
}

drow::AudioOscilloscope* OscilloscopeModuleDSP::getLeftOscilloscope()
{
    return &leftOscilloscope;
}

drow::AudioOscilloscope* OscilloscopeModuleDSP::getRightOscilloscope()
{
    return &rightOscilloscope;
}

OscilloscopeSettings OscilloscopeModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
    OscilloscopeSettings settings;
    settings.hZoom = apvts.getRawParameterValue("Oscilloscope H Zoom " + std::to_string(parameterNumber))->load();
    settings.vZoom = apvts.getRawParameterValue("Oscilloscope V Zoom " + std::to_string(parameterNumber))->load();
    settings.bypassed = apvts.getRawParameterValue("Oscilloscope Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;

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
            juce::NormalisableRange<float>(0.f, 2.f, 0.01f), 1.f, "Oscilloscope " + std::to_string(i))));
        // bypass button
        layout.add(std::make_unique<AudioParameterBool>("Oscilloscope Bypassed " + std::to_string(i), "Oscilloscope Bypassed " + std::to_string(i), 
            false, "Oscilloscope " + std::to_string(i)));
    }
}

void OscilloscopeModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, getChainPosition());
    bypassed = settings.bypassed;
    leftOscilloscope.setHorizontalZoom(settings.hZoom);
    leftOscilloscope.setVerticalZoom(settings.vZoom);
    rightOscilloscope.setHorizontalZoom(settings.hZoom);
    rightOscilloscope.setVerticalZoom(settings.vZoom);
}

void OscilloscopeModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    leftOscilloscope.clear();
    rightOscilloscope.clear();
    updateDSPState(sampleRate);
}

void OscilloscopeModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    updateDSPState(sampleRate);
    if (!bypassed) {
        leftOscilloscope.processBlock(buffer.getReadPointer(0), buffer.getNumSamples());
        rightOscilloscope.processBlock(buffer.getReadPointer(1), buffer.getNumSamples());
    }
}

//==============================================================================

/* OscilloscopeModule GUI */

//==============================================================================

OscilloscopeModuleGUI::OscilloscopeModuleGUI(PluginState& p, drow::AudioOscilloscope* _leftOscilloscope, drow::AudioOscilloscope* _rightOscilloscope, unsigned int parameterNumber)
    : GUIModule(), pluginState(p), leftOscilloscope(_leftOscilloscope), rightOscilloscope(_rightOscilloscope),
    hZoomSlider(*pluginState.apvts.getParameter("Oscilloscope H Zoom " + std::to_string(parameterNumber)), ""),
    vZoomSlider(*pluginState.apvts.getParameter("Oscilloscope V Zoom " + std::to_string(parameterNumber)), ""),
    hZoomSliderAttachment(pluginState.apvts, "Oscilloscope H Zoom " + std::to_string(parameterNumber), hZoomSlider),
    vZoomSliderAttachment(pluginState.apvts, "Oscilloscope V Zoom " + std::to_string(parameterNumber), vZoomSlider),
    bypassButtonAttachment(pluginState.apvts, "Oscilloscope Bypassed " + std::to_string(parameterNumber), bypassButton)
{
    // title setup
    title.setText("Oscilloscope", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    // labels
    hZoomLabel.setText("H Zoom", juce::dontSendNotification);
    hZoomLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    vZoomLabel.setText("V Zoom", juce::dontSendNotification);
    vZoomLabel.setFont(ModuleLookAndFeel::getLabelsFont());

    hZoomSlider.labels.add({ 0.f, "0" });
    hZoomSlider.labels.add({ 1.f, "1" });
    vZoomSlider.labels.add({ 0.f, "0" });
    vZoomSlider.labels.add({ 1.f, "2" });

    // bypass button
    bypassButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<OscilloscopeModuleGUI>(this);
    bypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->bypassButton.getToggleState();
            comp->handleParamCompsEnablement(bypassed);
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
                comp->leftOscilloscope->stopTimer();
                comp->rightOscilloscope->stopTimer();
            }
            else {
                comp->leftOscilloscope->startTimerHz(59);
                comp->rightOscilloscope->startTimerHz(59);
            }
            
        }
    };

    // tooltips
    bypassButton.setTooltip("Bypass this module");
    hZoomSlider.setTooltip("Adjust the horizontal zoom of the oscilloscope");
    vZoomSlider.setTooltip("Adjust the vertical zoom of the oscilloscope");
    freezeButton.setTooltip("Take a snapshot of the oscilloscope");

    if (!leftOscilloscope->isTimerRunning())
        leftOscilloscope->startTimerHz(59);
    if(!rightOscilloscope->isTimerRunning())
        rightOscilloscope->startTimerHz(59);

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    handleParamCompsEnablement(bypassButton.getToggleState());
}

OscilloscopeModuleGUI::~OscilloscopeModuleGUI()
{
    if (leftOscilloscope && leftOscilloscope->isTimerRunning()) {
        leftOscilloscope->stopTimer();
    }
    if (rightOscilloscope && rightOscilloscope->isTimerRunning()) {
        rightOscilloscope->stopTimer();
    }
    leftOscilloscope = nullptr;
    rightOscilloscope = nullptr;
    freezeButton.setLookAndFeel(nullptr);
    bypassButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> OscilloscopeModuleGUI::getAllComps()
{
    return {
        leftOscilloscope,
        rightOscilloscope,
        &title,
        &hZoomLabel,
        &vZoomLabel,
        &hZoomSlider,
        &vZoomSlider,
        &freezeButton,
        &bypassButton
    };
}

std::vector<juce::Component*> OscilloscopeModuleGUI::getParamComps()
{
    return {
        &hZoomSlider,
        &vZoomSlider,
        &freezeButton
    };
}

void OscilloscopeModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    hZoomSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    vZoomSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    freezeButton.setToggleState(false, juce::NotificationType::sendNotificationSync);
}

void OscilloscopeModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto hZoom = pluginState.apvts.getParameter("Oscilloscope H Zoom " + std::to_string(parameterNumber));
    auto vZoom = pluginState.apvts.getParameter("Oscilloscope V Zoom " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter("Oscilloscope Bypassed " + std::to_string(parameterNumber));

    hZoom->setValueNotifyingHost(hZoom->getDefaultValue());
    vZoom->setValueNotifyingHost(vZoom->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
    freezeButton.setToggleState(false, juce::NotificationType::dontSendNotification);
}

juce::Array<juce::var> OscilloscopeModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(hZoomSlider.getValue()));
    values.add(juce::var(vZoomSlider.getValue()));

    return values;
}

void OscilloscopeModuleGUI::paintOverChildren(Graphics& g)
{
    auto leftOscArea = leftOscilloscope->getBounds();
    auto rightOscArea = rightOscilloscope->getBounds();

    // drawing a separation line
    g.setColour(juce::Colours::white);
    g.drawLine(juce::Line<float>(leftOscArea.getBottomLeft().toFloat(), leftOscArea.getBottomRight().toFloat()), 1.5f);

    // L channel
    g.setColour(juce::Colours::darkgrey);
    auto left = juce::Point<float>((float)leftOscArea.getTopLeft().getX(), leftOscArea.getCentreY());
    auto right = juce::Point<float>((float)leftOscArea.getTopRight().getX(), leftOscArea.getCentreY());
    g.drawLine(juce::Line<float>(left, right), 1.f);
    //g.setColour(juce::Colours::white);
    // +1
    juce::Rectangle<int> labelArea;
    labelArea.setBounds(right.getX() - 14, leftOscArea.getTopRight().getY(), 14, 12);
    g.drawFittedText("+1", labelArea, juce::Justification::centred, 1);
    // 0
    labelArea.setBounds(right.getX() - 14, leftOscArea.getCentreY() - 14, 14, 12);
    g.drawFittedText("0", labelArea, juce::Justification::centred, 1);
    // -1
    labelArea.setBounds(right.getX() - 14, leftOscArea.getBottomRight().getY() - 13, 14, 12);
    g.drawFittedText("-1", labelArea, juce::Justification::centred, 1);
    // channel label
    g.setColour(juce::Colours::white.darker());
    labelArea.setBounds(left.getX() + 1, leftOscArea.getTopLeft().getY() + 1, 18, 16);
    g.drawFittedText("L", labelArea, juce::Justification::centred, 1);

    // R channel
    g.setColour(juce::Colours::darkgrey);
    left = juce::Point<float>((float)rightOscArea.getTopLeft().getX(), rightOscArea.getCentreY());
    right = juce::Point<float>((float)rightOscArea.getTopRight().getX(), rightOscArea.getCentreY());
    g.drawLine(juce::Line<float>(left, right), 1.f);
    //g.setColour(juce::Colours::white);
    // +1
    labelArea.setBounds(right.getX() - 14, rightOscArea.getTopRight().getY(), 14, 12);
    g.drawFittedText("+1", labelArea, juce::Justification::centred, 1);
    // 0
    labelArea.setBounds(right.getX() - 14, rightOscArea.getCentreY() - 14, 14, 12);
    g.drawFittedText("0", labelArea, juce::Justification::centred, 1);
    // -1
    labelArea.setBounds(right.getX() - 14, rightOscArea.getBottomRight().getY() - 13, 14, 12);
    g.drawFittedText("-1", labelArea, juce::Justification::centred, 1);
    // channel label
    g.setColour(juce::Colours::white.darker());
    labelArea.setBounds(left.getX() + 1, rightOscArea.getTopLeft().getY() + 2, 18, 16);
    g.drawFittedText("R", labelArea, juce::Justification::centred, 1);

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
    titleAndBypassArea.translate(0, 4);

    auto graphArea = oscilloscopeArea.removeFromTop(oscilloscopeArea.getHeight() * (2.f / 3.f));
    graphArea.reduce(10, 10);

    auto freezeArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * (1.f / 3.f));
    juce::Rectangle<int> freezeCorrectArea = freezeArea;
    freezeCorrectArea.reduce(50.f, 34.f);
    freezeCorrectArea.setCentre(freezeArea.getCentre());

    auto hZoomArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * (1.f / 2.f));
    auto hZoomLabelArea = hZoomArea.removeFromTop(14);

    auto vZoomArea = oscilloscopeArea;
    auto vZoomLabelArea = vZoomArea.removeFromTop(14);

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centredBottom);

    juce::Rectangle<int> renderArea;
    renderArea.setSize(hZoomArea.getHeight(), hZoomArea.getHeight());

    hZoomLabel.setBounds(hZoomLabelArea);
    hZoomLabel.setJustificationType(juce::Justification::centred);
    vZoomLabel.setBounds(vZoomLabelArea);
    vZoomLabel.setJustificationType(juce::Justification::centred);

    leftOscilloscope->setBounds(graphArea.removeFromTop(graphArea.getHeight() * (1.f / 2.f)));
    leftOscilloscope->startTimerHz(59);
    rightOscilloscope->setBounds(graphArea);
    rightOscilloscope->startTimerHz(59);

    renderArea.setCentre(hZoomArea.getCentre());
    renderArea.setY(hZoomArea.getTopLeft().getY());
    hZoomSlider.setBounds(renderArea);

    renderArea.setCentre(vZoomArea.getCentre());
    renderArea.setY(vZoomArea.getTopLeft().getY());
    vZoomSlider.setBounds(renderArea);

    freezeButton.setBounds(freezeCorrectArea);
}
