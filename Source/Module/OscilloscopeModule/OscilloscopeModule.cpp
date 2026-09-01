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
}

ScopeSampleBuffer& OscilloscopeModuleDSP::getSampleBuffer()
{
    return sampleBuffer;
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

void OscilloscopeModuleDSP::updateDSPState(double)
{
    // the view is applied when rendering, so the DSP only has to track the bypass here
    auto settings = getSettings(apvts, parameterNumber);
    bypassed = settings.bypassed;
}

void OscilloscopeModuleDSP::prepareToPlay(double sampleRate, int)
{
    updateDSPState(sampleRate);
}

void OscilloscopeModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&, double sampleRate)
{
    updateDSPState(sampleRate);

    if (!bypassed)
        sampleBuffer.push(buffer.getReadPointer(0), buffer.getReadPointer(1), buffer.getNumSamples());
}

//==============================================================================

/* OscilloscopeModule GUI */

//==============================================================================

OscilloscopeModuleGUI::OscilloscopeModuleGUI(PluginState& p, ScopeSampleBuffer& sampleBuffer,
    unsigned int parameterNumber)
    : GUIModule(), pluginState(p),
    scopeDisplay(sampleBuffer, moduleType_colors.at(ModuleType::Oscilloscope),
                 BiztortionTheme::analyzerRightChannel),
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
    setModuleHelp("Oscilloscope",
        "Shows the signal waveform, always synced to a rising zero crossing so repeating shapes "
        "stay still. Stereo combines the two channels, L / R shows them on separate lanes, and "
        "Freeze holds the current view. H Zoom sets the time window and V Zoom the amplitude "
        "scale: on the graph, scroll to magnify the amplitude, scroll sideways or hold Shift to "
        "stretch the time window, and double click to go back to the default view.");

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

    freezeButton.setClickingTogglesState(true);
    freezeButton.setLookAndFeel(&scopeControlsLnf);

    // Sync has no button any more: it is always on, because an unsynced trace is never the
    // useful view here. The parameter itself is kept so older sessions still load.
    syncButton.setClickingTogglesState(true);
    syncButton.setToggleState(true, juce::sendNotificationSync);
    freezeButton.setToggleState(false, juce::dontSendNotification);

    stereoButton.setClickingTogglesState(true);
    stereoButton.setModuleColours(moduleType_colors.at(ModuleType::Oscilloscope),
        moduleType_colors.at(ModuleType::Oscilloscope),
        BiztortionTheme::analyzerRightChannel);

    scopeControlsLnf.setColour(juce::TextButton::buttonColourId, juce::Colour(39, 42, 45));
    scopeControlsLnf.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    scopeControlsLnf.setColour(juce::TextButton::buttonOnColourId, moduleType_colors.at(ModuleType::Oscilloscope));
    scopeControlsLnf.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    freezeButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            // the samples are kept, so the frozen view can still be zoomed and panned
            comp->scopeDisplay.setFrozen(comp->freezeButton.getToggleState());
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

    for (auto* slider : { &hZoomSlider, &vZoomSlider })
    {
        slider->onValueChange = [safePtr]()
        {
            if (auto* comp = safePtr.getComponent())
                comp->updateScopeView();
        };
    }

    // zooming on the graph moves the knobs, so the gesture ends up in the parameters
    scopeDisplay.onZoomGesture = [safePtr](float horizontal, float vertical)
    {
        if (auto* comp = safePtr.getComponent())
        {
            // the sliders drive the scope's zoom directly, so there is no loop to guard against
            comp->hZoomSlider.setValue(horizontal, juce::sendNotificationSync);
            comp->vZoomSlider.setValue(vertical, juce::sendNotificationSync);
        }
    };


    // tooltips
    bypassButton.setTooltip("Bypass this module");
    hZoomSlider.setTooltip("Adjust the horizontal zoom of the oscilloscope");
    vZoomSlider.setTooltip("Adjust the vertical zoom of the oscilloscope");
    freezeButton.setTooltip("Take a snapshot of the oscilloscope");
    stereoButton.setTooltip("Switch between separate L/R traces and one averaged stereo waveform");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    synchroniseScopes();
    updateScopeLayout();
    updateScopeView();
    handleParamCompsEnablement(bypassButton.getToggleState());
}

OscilloscopeModuleGUI::~OscilloscopeModuleGUI()
{
    freezeButton.setLookAndFeel(nullptr);
    bypassButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> OscilloscopeModuleGUI::getAllComps()
{
    return {
        &scopeDisplay,
        &title,
        &hZoomLabel,
        &vZoomLabel,
        &hZoomSlider,
        &vZoomSlider,
        &freezeButton,
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
        &stereoButton
    };
}

void OscilloscopeModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    hZoomSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    vZoomSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    // sync is always on, but the value still has to be consumed to stay in step with getParamValues()
    ++value;
    syncButton.setToggleState(true, juce::NotificationType::sendNotificationSync);
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

void OscilloscopeModuleGUI::paintOverChildren(Graphics&)
{
    // the scope display draws its own references, traces and labels
}

void OscilloscopeModuleGUI::synchroniseScopes()
{
    // the display always aligns its window to a rising zero crossing
}

void OscilloscopeModuleGUI::updateScopeView()
{
    scopeDisplay.setSampleRate(pluginState.audioProcessor.getSampleRate());
    scopeDisplay.setView(static_cast<float>(hZoomSlider.getValue()),
                         static_cast<float>(vZoomSlider.getValue()));
}

void OscilloscopeModuleGUI::updateScopeLayout()
{
    scopeDisplay.setStereo(stereoButton.getToggleState());
    resized();
    repaint();
}

void OscilloscopeModuleGUI::resized()
{
    auto oscilloscopeArea = getContentRenderArea();

    auto titleAndBypassArea = oscilloscopeArea.removeFromTop(36);
    const auto titleArea = titleAndBypassArea;
    bypassButton.setBounds(juce::Rectangle<int>(headerButtonSize, headerButtonSize).withCentre(
        { titleAndBypassArea.getX() + 20, titleAndBypassArea.getCentreY() }));

    // the view switch lives in the header, leaving the help button its own slot on the right
    titleAndBypassArea.removeFromRight(headerHelpButtonArea);
    stereoButton.setBounds(titleAndBypassArea.removeFromRight(78).reduced(0, 3));

    // the controls row keeps enough height for three full size buttons even at the smallest
    // allowed window size, so the graph takes what is left rather than a fixed share
    // the graph keeps ~68% of the module height; the controls row still gets enough space for
    // full size buttons and knobs at the smallest allowed window size
    const auto controlsHeight = juce::jlimit(92, 132, juce::roundToInt(oscilloscopeArea.getHeight() * 0.28f));
    auto graphArea = oscilloscopeArea.removeFromTop(juce::jmax(60, oscilloscopeArea.getHeight() - controlsHeight));
    graphArea.reduce(10, 8);

    auto buttonArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * 0.4f).reduced(12, 6);
    auto hZoomArea = oscilloscopeArea.removeFromLeft(oscilloscopeArea.getWidth() * (1.f / 2.f));
    auto hZoomLabelArea = hZoomArea.removeFromTop(14);

    auto vZoomArea = oscilloscopeArea;
    auto vZoomLabelArea = vZoomArea.removeFromTop(14);

    title.setBounds(titleArea);
    title.setJustificationType(juce::Justification::centred);

    juce::Rectangle<int> renderArea;
    renderArea.setSize(hZoomArea.getHeight(), hZoomArea.getHeight());

    hZoomLabel.setBounds(hZoomLabelArea);
    hZoomLabel.setJustificationType(juce::Justification::centred);
    vZoomLabel.setBounds(vZoomLabelArea);
    vZoomLabel.setJustificationType(juce::Justification::centred);

    scopeDisplay.setBounds(graphArea);

    renderArea.setCentre(hZoomArea.getCentre());
    renderArea.setY(hZoomArea.getTopLeft().getY());
    hZoomSlider.setBounds(renderArea);

    renderArea.setCentre(vZoomArea.getCentre());
    renderArea.setY(vZoomArea.getTopLeft().getY());
    vZoomSlider.setBounds(renderArea);

    // keep these the same size as the other text buttons in the plugin (e.g. the equalizer's
    // analyzer view button) instead of stretching or squashing them to fit the column
    const auto buttonHeight = juce::jlimit(24, 30, buttonArea.getHeight() / 2);
    const auto buttonWidth = juce::jmin(84, buttonArea.getWidth());
    freezeButton.setBounds(buttonArea.withSizeKeepingCentre(buttonWidth, buttonHeight));
}
