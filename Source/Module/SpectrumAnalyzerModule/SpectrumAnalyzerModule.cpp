/*
  ==============================================================================

    SpectrumAnalyzerModule.cpp

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

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

#include "SpectrumAnalyzerModule.h"

//==============================================================================

/* SpectrumAnalyzerModule DSP */

//==============================================================================

SpectrumAnalyzerModuleDSP::SpectrumAnalyzerModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts)
{
}

SpectrumAnalyzerSettings SpectrumAnalyzerModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
    SpectrumAnalyzerSettings settings;
    settings.hZoom = apvts.getRawParameterValue("SpectrumAnalyzer H Zoom " + std::to_string(parameterNumber))->load();
    settings.vZoom = apvts.getRawParameterValue("SpectrumAnalyzer V Zoom " + std::to_string(parameterNumber))->load();
    settings.stereoView = apvts.getRawParameterValue("SpectrumAnalyzer Stereo " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.bypassed = apvts.getRawParameterValue("SpectrumAnalyzer Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;

    return settings;
}

void SpectrumAnalyzerModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for (int i = 1; i < 9; ++i) {
        layout.add(std::make_unique<juce::AudioParameterFloat>("SpectrumAnalyzer H Zoom " + std::to_string(i),
            "SpectrumAnalyzer H Zoom " + std::to_string(i),
            juce::NormalisableRange<float>(1.f, 24.f, 0.1f, 0.4f), 1.f,
            "SpectrumAnalyzer " + std::to_string(i)));
        layout.add(std::make_unique<juce::AudioParameterFloat>("SpectrumAnalyzer V Zoom " + std::to_string(i),
            "SpectrumAnalyzer V Zoom " + std::to_string(i),
            juce::NormalisableRange<float>(1.f, 4.f, 0.05f), 1.f,
            "SpectrumAnalyzer " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("SpectrumAnalyzer Stereo " + std::to_string(i),
            "SpectrumAnalyzer Stereo " + std::to_string(i),
            false, "SpectrumAnalyzer " + std::to_string(i)));
        // bypass button
        layout.add(std::make_unique<AudioParameterBool>("SpectrumAnalyzer Bypassed " + std::to_string(i),
            "SpectrumAnalyzer Bypassed " + std::to_string(i),
            false, "SpectrumAnalyzer " + std::to_string(i)));
    }
}

void SpectrumAnalyzerModuleDSP::updateDSPState(double)
{
    auto settings = getSettings(apvts, parameterNumber);
    bypassed = settings.bypassed;
}

void SpectrumAnalyzerModuleDSP::prepareToPlay(double sampleRate, int)
{
    updateDSPState(sampleRate);
}

void SpectrumAnalyzerModuleDSP::processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double sampleRate)
{
    // the module only observes the signal: the FFT FIFOs are filled by the processor before
    // this call, so there is nothing to do to the buffer itself
    updateDSPState(sampleRate);
}

//==============================================================================

/* SpectrumAnalyzerModule GUI */

//==============================================================================

namespace
{
    FFTAnalyzerComponent::Options makeAnalyzerOptions()
    {
        const auto accent = moduleType_colors.at(ModuleType::SpectrumAnalyzer);

        FFTAnalyzerComponent::Options options;
        options.moduleType = ModuleType::SpectrumAnalyzer;
        options.leftColour = accent;
        // warm tone for the right channel so an L / R comparison is easy to read
        options.rightColour = BiztortionTheme::analyzerRightChannel;
        options.stereoColour = accent;
        // no filter response is drawn here, so show a plain dBFS scale and fill the curve
        options.showFilterGainScale = false;
        options.fillUnderCurve = true;

        return options;
    }
}

SpectrumAnalyzerModuleGUI::SpectrumAnalyzerModuleGUI(PluginState& p, unsigned int parameterNumber)
    : GUIModule(), pluginState(p),
    hZoomSlider(*pluginState.apvts.getParameter("SpectrumAnalyzer H Zoom " + std::to_string(parameterNumber)), "x"),
    vZoomSlider(*pluginState.apvts.getParameter("SpectrumAnalyzer V Zoom " + std::to_string(parameterNumber)), "x"),
    hZoomSliderAttachment(pluginState.apvts, "SpectrumAnalyzer H Zoom " + std::to_string(parameterNumber), hZoomSlider),
    vZoomSliderAttachment(pluginState.apvts, "SpectrumAnalyzer V Zoom " + std::to_string(parameterNumber), vZoomSlider),
    bypassButtonAttachment(pluginState.apvts, "SpectrumAnalyzer Bypassed " + std::to_string(parameterNumber), bypassButton),
    analyzerViewButtonAttachment(pluginState.apvts, "SpectrumAnalyzer Stereo " + std::to_string(parameterNumber), analyzerViewButton),
    fftAnalyzerComponent(p, parameterNumber, makeAnalyzerOptions())
{
    // title setup
    title.setText("Spectrum Analyzer", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    moduleColor = moduleType_colors.at(ModuleType::SpectrumAnalyzer);
    setModuleHelp("Spectrum Analyzer",
        "Shows the frequency content of the signal at this point of the chain, from 20 Hz to 20 kHz "
        "with levels between 0 and -48 dB. Use Stereo for one averaged curve or L / R to compare the "
        "two channels. H Zoom magnifies the frequency axis and V Zoom the level axis: while zoomed "
        "in, drag the graph to move along it, use the mouse wheel to zoom around the pointer and "
        "double click it to jump back to the start. Freeze holds the current spectrum on screen.");

    // labels
    hZoomLabel.setText("H Zoom", juce::dontSendNotification);
    hZoomLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    vZoomLabel.setText("V Zoom", juce::dontSendNotification);
    vZoomLabel.setFont(ModuleLookAndFeel::getLabelsFont());

    hZoomSlider.labels.add({ 0.f, "1x" });
    hZoomSlider.labels.add({ 1.f, "24x" });
    vZoomSlider.labels.add({ 0.f, "1x" });
    vZoomSlider.labels.add({ 1.f, "4x" });

    // bypass button
    lnf.color = moduleType_colors.at(ModuleType::SpectrumAnalyzer);
    bypassButton.setLookAndFeel(&lnf);

    analyzerViewButton.setClickingTogglesState(true);
    {
        const auto analyzerOptions = makeAnalyzerOptions();
        analyzerViewButton.setModuleColours(moduleType_colors.at(ModuleType::SpectrumAnalyzer),
            analyzerOptions.leftColour, analyzerOptions.rightColour);
    }

    auto safePtr = juce::Component::SafePointer<SpectrumAnalyzerModuleGUI>(this);

    bypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->bypassButton.getToggleState();
            comp->handleParamCompsEnablement(bypassed);
            // a bypassed analyzer stops updating instead of showing a stale spectrum
            comp->fftAnalyzerComponent.toggleFFTanaysis(!bypassed);
            comp->updateControlEnablement();
        }
    };

    analyzerViewButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
            comp->updateAnalyzerView();
    };

    freezeButton.setClickingTogglesState(true);
    freezeButton.setToggleState(false, juce::dontSendNotification);
    freezeButton.setLookAndFeel(&analyzerControlsLnf);
    analyzerControlsLnf.setColour(juce::TextButton::buttonColourId, BiztortionTheme::surfaceRaised);
    analyzerControlsLnf.setColour(juce::TextButton::textColourOffId, BiztortionTheme::text);
    analyzerControlsLnf.setColour(juce::TextButton::buttonOnColourId, moduleType_colors.at(ModuleType::SpectrumAnalyzer));
    analyzerControlsLnf.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

    freezeButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
            comp->fftAnalyzerComponent.setFrozen(comp->freezeButton.getToggleState());
    };

    hZoomSlider.onValueChange = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
            comp->updateAnalyzerZoom();
    };

    vZoomSlider.onValueChange = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
            comp->updateAnalyzerZoom();
    };

    // zooming on the graph itself (wheel or trackpad pinch) has to move the knobs too, so the
    // gesture ends up in the parameters like any other edit
    fftAnalyzerComponent.onViewZoomChangedByGesture = [safePtr](float horizontal, float vertical)
    {
        if (auto* comp = safePtr.getComponent())
        {
            // the graph has already applied the zoom, so don't push it straight back
            const juce::ScopedValueSetter<bool> guard(comp->applyingZoomFromGesture, true);
            comp->hZoomSlider.setValue(horizontal, juce::sendNotificationSync);
            comp->vZoomSlider.setValue(vertical, juce::sendNotificationSync);
        }
    };

    // tooltips
    bypassButton.setTooltip("Bypass this module");
    analyzerViewButton.setTooltip("Switch between separate left/right spectra and one stereo-average spectrum");
    freezeButton.setTooltip("Hold the current spectrum on screen");
    hZoomSlider.setTooltip("Magnify the frequency axis. Zoom on the graph with the wheel or a "
                           "pinch, then drag or two finger scroll to move along it");
    vZoomSlider.setTooltip("Magnify the level axis around 0 dB");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    fftAnalyzerComponent.toggleFFTanaysis(!bypassButton.getToggleState());
    updateAnalyzerView();
    updateAnalyzerZoom();
    updateControlEnablement();
    handleParamCompsEnablement(bypassButton.getToggleState());
}

SpectrumAnalyzerModuleGUI::~SpectrumAnalyzerModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
    freezeButton.setLookAndFeel(nullptr);
}

void SpectrumAnalyzerModuleGUI::updateAnalyzerZoom()
{
    // when the change came from a gesture on the graph the view is already where the user put
    // it (anchored on the pointer); pushing it back would re-centre and fight the gesture
    if (applyingZoomFromGesture)
        return;

    fftAnalyzerComponent.setViewZoom(static_cast<float>(hZoomSlider.getValue()),
                                     static_cast<float>(vZoomSlider.getValue()));
}

void SpectrumAnalyzerModuleGUI::updateAnalyzerView()
{
    const auto stereo = analyzerViewButton.getToggleState();
    analyzerViewButton.setButtonText(stereo ? "Stereo" : "L / R");
    fftAnalyzerComponent.setStereoView(stereo);
}

void SpectrumAnalyzerModuleGUI::updateControlEnablement()
{
    // the view switch only has an effect while the analyzer is actually running
    analyzerViewButton.setEnabled(!bypassButton.getToggleState());
}

std::vector<juce::Component*> SpectrumAnalyzerModuleGUI::getAllComps()
{
    return {
        &title,
        &fftAnalyzerComponent,
        &hZoomLabel,
        &vZoomLabel,
        &hZoomSlider,
        &vZoomSlider,
        &bypassButton,
        &analyzerViewButton,
        &freezeButton
    };
}

std::vector<juce::Component*> SpectrumAnalyzerModuleGUI::getParamComps()
{
    return {
        &hZoomSlider,
        &vZoomSlider,
        &analyzerViewButton,
        &freezeButton
    };
}

void SpectrumAnalyzerModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();

    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    analyzerViewButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    hZoomSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    vZoomSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    freezeButton.setToggleState(false, juce::NotificationType::sendNotificationSync);

    updateAnalyzerView();
    updateAnalyzerZoom();
    updateControlEnablement();
}

void SpectrumAnalyzerModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto stereo = pluginState.apvts.getParameter("SpectrumAnalyzer Stereo " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter("SpectrumAnalyzer Bypassed " + std::to_string(parameterNumber));
    auto hZoom = pluginState.apvts.getParameter("SpectrumAnalyzer H Zoom " + std::to_string(parameterNumber));
    auto vZoom = pluginState.apvts.getParameter("SpectrumAnalyzer V Zoom " + std::to_string(parameterNumber));

    stereo->setValueNotifyingHost(stereo->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
    hZoom->setValueNotifyingHost(hZoom->getDefaultValue());
    vZoom->setValueNotifyingHost(vZoom->getDefaultValue());
}

juce::Array<juce::var> SpectrumAnalyzerModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(analyzerViewButton.getToggleState()));
    values.add(juce::var(hZoomSlider.getValue()));
    values.add(juce::var(vZoomSlider.getValue()));

    return values;
}

void SpectrumAnalyzerModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void SpectrumAnalyzerModuleGUI::resized()
{
    auto analyzerArea = getContentRenderArea();

    auto titleAndBypassArea = analyzerArea.removeFromTop(36);
    const auto titleArea = titleAndBypassArea;
    bypassButton.setBounds(juce::Rectangle<int>(headerButtonSize, headerButtonSize).withCentre(
        { titleAndBypassArea.getX() + 20, titleAndBypassArea.getCentreY() }));

    // leave the help button its own slot plus an even gap on both of its sides
    titleAndBypassArea.removeFromRight(headerHelpButtonArea);
    analyzerViewButton.setBounds(titleAndBypassArea.removeFromRight(78).reduced(0, 3));

    title.setBounds(titleArea);
    title.setJustificationType(juce::Justification::centred);

    // same graph / controls proportions as the scope module
    // the graph keeps ~68% of the module height; the controls row still gets enough space for
    // full size buttons and knobs at the smallest allowed window size
    const auto controlsHeight = juce::jlimit(92, 132, juce::roundToInt(analyzerArea.getHeight() * 0.28f));
    auto graphArea = analyzerArea.removeFromTop(juce::jmax(60, analyzerArea.getHeight() - controlsHeight));
    fftAnalyzerComponent.setBounds(graphArea.reduced(8, 6));

    auto buttonArea = analyzerArea.removeFromLeft(analyzerArea.getWidth() * 0.4f).reduced(12, 6);
    auto hZoomArea = analyzerArea.removeFromLeft(analyzerArea.getWidth() * (1.f / 2.f));
    auto hZoomLabelArea = hZoomArea.removeFromTop(14);
    auto vZoomArea = analyzerArea;
    auto vZoomLabelArea = vZoomArea.removeFromTop(14);

    hZoomLabel.setBounds(hZoomLabelArea);
    hZoomLabel.setJustificationType(juce::Justification::centred);
    vZoomLabel.setBounds(vZoomLabelArea);
    vZoomLabel.setJustificationType(juce::Justification::centred);

    juce::Rectangle<int> renderArea;
    renderArea.setSize(hZoomArea.getHeight(), hZoomArea.getHeight());

    renderArea.setCentre(hZoomArea.getCentre());
    renderArea.setY(hZoomArea.getTopLeft().getY());
    hZoomSlider.setBounds(renderArea);

    renderArea.setCentre(vZoomArea.getCentre());
    renderArea.setY(vZoomArea.getTopLeft().getY());
    vZoomSlider.setBounds(renderArea);

    const auto buttonHeight = juce::jlimit(24, 30, buttonArea.getHeight() / 2);
    const auto buttonWidth = juce::jmin(84, buttonArea.getWidth());
    freezeButton.setBounds(buttonArea.withSizeKeepingCentre(buttonWidth, buttonHeight));
}
