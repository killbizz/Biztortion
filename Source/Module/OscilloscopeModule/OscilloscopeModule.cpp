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
    stereoOscilloscope.clear();
}

drow::AudioOscilloscope* OscilloscopeModuleDSP::getLeftOscilloscope()
{
    return &leftOscilloscope;
}

drow::AudioOscilloscope* OscilloscopeModuleDSP::getRightOscilloscope()
{
    return &rightOscilloscope;
}

drow::AudioOscilloscope* OscilloscopeModuleDSP::getStereoOscilloscope()
{
    return &stereoOscilloscope;
}

OscilloscopeSettings OscilloscopeModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
    OscilloscopeSettings settings;
    settings.hZoom = apvts.getRawParameterValue("Oscilloscope H Zoom " + std::to_string(parameterNumber))->load();
    settings.vZoom = apvts.getRawParameterValue("Oscilloscope V Zoom " + std::to_string(parameterNumber))->load();
    settings.syncEnabled = apvts.getRawParameterValue("Oscilloscope Sync " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.stereoView = apvts.getRawParameterValue("Oscilloscope Overlay " + std::to_string(parameterNumber))->load() > 0.5f;
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
        layout.add(std::make_unique<AudioParameterBool>("Oscilloscope Sync " + std::to_string(i), "Oscilloscope Sync " + std::to_string(i),
            true, "Oscilloscope " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("Oscilloscope Overlay " + std::to_string(i), "Oscilloscope Stereo " + std::to_string(i),
            true, "Oscilloscope " + std::to_string(i)));
        // bypass button
        layout.add(std::make_unique<AudioParameterBool>("Oscilloscope Bypassed " + std::to_string(i), "Oscilloscope Bypassed " + std::to_string(i), 
            false, "Oscilloscope " + std::to_string(i)));
    }
}

void OscilloscopeModuleDSP::updateDSPState(double sampleRate)
{
    auto settings = getSettings(apvts, parameterNumber);
    bypassed = settings.bypassed;
    leftOscilloscope.setHorizontalZoom(settings.hZoom);
    leftOscilloscope.setVerticalZoom(settings.vZoom);
    rightOscilloscope.setHorizontalZoom(settings.hZoom);
    rightOscilloscope.setVerticalZoom(settings.vZoom);
    stereoOscilloscope.setHorizontalZoom(settings.hZoom);
    stereoOscilloscope.setVerticalZoom(settings.vZoom);
    leftOscilloscope.setTriggerEnabled(settings.syncEnabled);
    rightOscilloscope.setTriggerEnabled(settings.syncEnabled);
    stereoOscilloscope.setTriggerEnabled(settings.syncEnabled);
}

void OscilloscopeModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    leftOscilloscope.clear();
    rightOscilloscope.clear();
    stereoOscilloscope.clear();
    updateDSPState(sampleRate);
}

void OscilloscopeModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    updateDSPState(sampleRate);
    if (!bypassed) {
        leftOscilloscope.processBlock(buffer.getReadPointer(0), buffer.getNumSamples());
        rightOscilloscope.processBlock(buffer.getReadPointer(1), buffer.getNumSamples());
        stereoOscilloscope.processStereoBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), buffer.getNumSamples());
    }
}

//==============================================================================

/* OscilloscopeModule GUI */

//==============================================================================

OscilloscopeModuleGUI::OscilloscopeModuleGUI(PluginState& p, drow::AudioOscilloscope* _leftOscilloscope,
    drow::AudioOscilloscope* _rightOscilloscope, drow::AudioOscilloscope* _stereoOscilloscope,
    unsigned int parameterNumber)
    : GUIModule(), pluginState(p), leftOscilloscope(_leftOscilloscope),
    rightOscilloscope(_rightOscilloscope), stereoOscilloscope(_stereoOscilloscope),
    hZoomSlider(*pluginState.apvts.getParameter("Oscilloscope H Zoom " + std::to_string(parameterNumber)), ""),
    vZoomSlider(*pluginState.apvts.getParameter("Oscilloscope V Zoom " + std::to_string(parameterNumber)), ""),
    hZoomSliderAttachment(pluginState.apvts, "Oscilloscope H Zoom " + std::to_string(parameterNumber), hZoomSlider),
    vZoomSliderAttachment(pluginState.apvts, "Oscilloscope V Zoom " + std::to_string(parameterNumber), vZoomSlider),
    syncButtonAttachment(pluginState.apvts, "Oscilloscope Sync " + std::to_string(parameterNumber), syncButton),
    stereoButtonAttachment(pluginState.apvts, "Oscilloscope Overlay " + std::to_string(parameterNumber), stereoButton),
    bypassButtonAttachment(pluginState.apvts, "Oscilloscope Bypassed " + std::to_string(parameterNumber), bypassButton)
{
    // title setup
    title.setText("Scope", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    moduleColor = moduleType_colors.at(ModuleType::Oscilloscope);

    leftOscilloscope->setTraceColour(moduleType_colors.at(ModuleType::Oscilloscope));
    rightOscilloscope->setTraceColour(moduleType_colors.at(ModuleType::Oscilloscope).interpolatedWith(juce::Colours::white, 0.55f));
    stereoOscilloscope->setTraceColour(moduleType_colors.at(ModuleType::Oscilloscope));

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
    lnf.color = moduleType_colors.at(ModuleType::Oscilloscope);
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

    for (auto* button : { &freezeButton, &syncButton, &stereoButton })
    {
        button->setClickingTogglesState(true);
        button->setLookAndFeel(&scopeControlsLnf);
    }
    freezeButton.setToggleState(false, juce::dontSendNotification);

    scopeControlsLnf.setColour(juce::TextButton::buttonColourId, juce::Colour(39, 42, 45));
    scopeControlsLnf.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    scopeControlsLnf.setColour(juce::TextButton::buttonOnColourId, moduleType_colors.at(ModuleType::Oscilloscope));
    scopeControlsLnf.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    freezeButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto freeze = comp->freezeButton.getToggleState();
            if (freeze) {
                comp->leftOscilloscope->stopTimer();
                comp->rightOscilloscope->stopTimer();
                comp->stereoOscilloscope->stopTimer();
            }
            else {
                comp->leftOscilloscope->startTimerHz(59);
                comp->rightOscilloscope->startTimerHz(59);
                comp->stereoOscilloscope->startTimerHz(59);
            }
            
        }
    };

    syncButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
            comp->synchroniseScopes();
    };

    stereoButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
            comp->updateScopeLayout();
    };

    // tooltips
    bypassButton.setTooltip("Bypass this module");
    hZoomSlider.setTooltip("Adjust the horizontal zoom of the oscilloscope");
    vZoomSlider.setTooltip("Adjust the vertical zoom of the oscilloscope");
    freezeButton.setTooltip("Take a snapshot of the oscilloscope");
    syncButton.setTooltip("Lock each refresh to a rising zero crossing for a stable waveform");
    stereoButton.setTooltip("Switch between separate L/R traces and one averaged stereo waveform");

    if (!leftOscilloscope->isTimerRunning())
        leftOscilloscope->startTimerHz(59);
    if(!rightOscilloscope->isTimerRunning())
        rightOscilloscope->startTimerHz(59);
    if(!stereoOscilloscope->isTimerRunning())
        stereoOscilloscope->startTimerHz(59);

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    synchroniseScopes();
    updateScopeLayout();
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
    if (stereoOscilloscope && stereoOscilloscope->isTimerRunning()) {
        stereoOscilloscope->stopTimer();
    }
    leftOscilloscope = nullptr;
    rightOscilloscope = nullptr;
    stereoOscilloscope = nullptr;
    freezeButton.setLookAndFeel(nullptr);
    syncButton.setLookAndFeel(nullptr);
    stereoButton.setLookAndFeel(nullptr);
    bypassButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> OscilloscopeModuleGUI::getAllComps()
{
    return {
        leftOscilloscope,
        rightOscilloscope,
        stereoOscilloscope,
        &title,
        &hZoomLabel,
        &vZoomLabel,
        &hZoomSlider,
        &vZoomSlider,
        &freezeButton,
        &syncButton,
        &stereoButton,
        &bypassButton
    };
}

std::vector<juce::Component*> OscilloscopeModuleGUI::getParamComps()
{
    return {
        &hZoomSlider,
        &vZoomSlider,
        &freezeButton,
        &syncButton,
        &stereoButton
    };
}

void OscilloscopeModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    hZoomSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    vZoomSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    syncButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    stereoButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    freezeButton.setToggleState(false, juce::NotificationType::sendNotificationSync);
}

void OscilloscopeModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto hZoom = pluginState.apvts.getParameter("Oscilloscope H Zoom " + std::to_string(parameterNumber));
    auto vZoom = pluginState.apvts.getParameter("Oscilloscope V Zoom " + std::to_string(parameterNumber));
    auto sync = pluginState.apvts.getParameter("Oscilloscope Sync " + std::to_string(parameterNumber));
    auto stereo = pluginState.apvts.getParameter("Oscilloscope Overlay " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter("Oscilloscope Bypassed " + std::to_string(parameterNumber));

    hZoom->setValueNotifyingHost(hZoom->getDefaultValue());
    vZoom->setValueNotifyingHost(vZoom->getDefaultValue());
    sync->setValueNotifyingHost(sync->getDefaultValue());
    stereo->setValueNotifyingHost(stereo->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
    freezeButton.setToggleState(false, juce::NotificationType::dontSendNotification);
}

juce::Array<juce::var> OscilloscopeModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(hZoomSlider.getValue()));
    values.add(juce::var(vZoomSlider.getValue()));
    values.add(juce::var(syncButton.getToggleState()));
    values.add(juce::var(stereoButton.getToggleState()));

    return values;
}

void OscilloscopeModuleGUI::paintOverChildren(Graphics& g)
{
    auto leftOscArea = leftOscilloscope->getBounds();
    auto rightOscArea = rightOscilloscope->getBounds();
    const auto stereo = stereoButton.getToggleState();

    if (stereo)
    {
        const auto stereoArea = stereoOscilloscope->getBounds();
        g.setColour(juce::Colours::darkgrey);
        const auto left = juce::Point<float>((float)stereoArea.getX(), (float)stereoArea.getCentreY());
        const auto right = juce::Point<float>((float)stereoArea.getRight(), (float)stereoArea.getCentreY());
        g.drawLine({ left, right }, 1.0f);

        juce::Rectangle<int> labelArea;
        g.setColour(moduleType_colors.at(ModuleType::Oscilloscope));
        labelArea.setBounds(stereoArea.getX() + 2, stereoArea.getY() + 2, 52, 16);
        g.drawFittedText("Stereo", labelArea, juce::Justification::centredLeft, 1);

        g.setColour(juce::Colours::darkgrey);
        labelArea.setBounds(stereoArea.getRight() - 14, stereoArea.getY(), 14, 12);
        g.drawFittedText("+1", labelArea, juce::Justification::centred, 1);
        labelArea.setBounds(stereoArea.getRight() - 14, stereoArea.getCentreY() - 14, 14, 12);
        g.drawFittedText("0", labelArea, juce::Justification::centred, 1);
        labelArea.setBounds(stereoArea.getRight() - 14, stereoArea.getBottom() - 13, 14, 12);
        g.drawFittedText("-1", labelArea, juce::Justification::centred, 1);
        return;
    }

    // drawing a separation line
    g.setColour(juce::Colour(66, 70, 74));
    g.drawLine(juce::Line<float>(leftOscArea.getBottomLeft().toFloat(), leftOscArea.getBottomRight().toFloat()), 1.0f);

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
    g.setColour(moduleType_colors.at(ModuleType::Oscilloscope));
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
    g.setColour(moduleType_colors.at(ModuleType::Oscilloscope).interpolatedWith(juce::Colours::white, 0.55f));
    labelArea.setBounds(left.getX() + 1, rightOscArea.getTopLeft().getY() + 2, 18, 16);
    g.drawFittedText("R", labelArea, juce::Justification::centred, 1);

}

void OscilloscopeModuleGUI::synchroniseScopes()
{
    const auto enabled = syncButton.getToggleState();
    leftOscilloscope->setTriggerEnabled(enabled);
    rightOscilloscope->setTriggerEnabled(enabled);
    stereoOscilloscope->setTriggerEnabled(enabled);
    leftOscilloscope->synchronise();
    rightOscilloscope->synchronise();
    stereoOscilloscope->synchronise();
}

void OscilloscopeModuleGUI::updateScopeLayout()
{
    const auto stereo = stereoButton.getToggleState();
    leftOscilloscope->setVisible(!stereo);
    rightOscilloscope->setVisible(!stereo);
    stereoOscilloscope->setVisible(stereo);
    leftOscilloscope->synchronise();
    rightOscilloscope->synchronise();
    stereoOscilloscope->synchronise();
    resized();
    repaint();
}

void OscilloscopeModuleGUI::resized()
{
    auto oscilloscopeArea = getContentRenderArea();

    auto titleAndBypassArea = oscilloscopeArea.removeFromTop(36);
    bypassButton.setBounds(juce::Rectangle<int>(28, 28).withCentre(
        { titleAndBypassArea.getX() + 20, titleAndBypassArea.getCentreY() }));

    auto graphArea = oscilloscopeArea.removeFromTop(oscilloscopeArea.getHeight() * 0.68f);
    graphArea.reduce(10, 8);

    auto buttonArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * 0.4f).reduced(12, 12);
    auto hZoomArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * (1.f / 2.f));
    auto hZoomLabelArea = hZoomArea.removeFromTop(14);

    auto vZoomArea = oscilloscopeArea;
    auto vZoomLabelArea = vZoomArea.removeFromTop(14);

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centred);

    juce::Rectangle<int> renderArea;
    renderArea.setSize(hZoomArea.getHeight(), hZoomArea.getHeight());

    hZoomLabel.setBounds(hZoomLabelArea);
    hZoomLabel.setJustificationType(juce::Justification::centred);
    vZoomLabel.setBounds(vZoomLabelArea);
    vZoomLabel.setJustificationType(juce::Justification::centred);

    if (stereoButton.getToggleState())
    {
        stereoOscilloscope->setBounds(graphArea);
    }
    else
    {
        leftOscilloscope->setBounds(graphArea.removeFromTop(graphArea.getHeight() * (1.f / 2.f)));
        rightOscilloscope->setBounds(graphArea);
    }
    leftOscilloscope->startTimerHz(59);
    rightOscilloscope->startTimerHz(59);
    stereoOscilloscope->startTimerHz(59);

    renderArea.setCentre(hZoomArea.getCentre());
    renderArea.setY(hZoomArea.getTopLeft().getY());
    hZoomSlider.setBounds(renderArea);

    renderArea.setCentre(vZoomArea.getCentre());
    renderArea.setY(vZoomArea.getTopLeft().getY());
    vZoomSlider.setBounds(renderArea);

    const auto buttonHeight = juce::jmin(32, buttonArea.getHeight() / 3);
    freezeButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(5);
    syncButton.setBounds(buttonArea.removeFromTop(buttonHeight));
    buttonArea.removeFromTop(5);
    stereoButton.setBounds(buttonArea.removeFromTop(buttonHeight));
}
