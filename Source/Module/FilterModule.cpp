/*
  ==============================================================================

    FilterModule.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2021 Charles Schiermeyer
    Content: the original Equalizer Algorithm
    Source: https://github.com/matkatmusic/SimpleEQ

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

#include "FilterModule.h"

//==============================================================================

/* FilterModule DSP */

//==============================================================================

FilterModuleDSP::FilterModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
    : DSPModule(_apvts) {
}

void updateCoefficients(Coefficients& old, const Coefficients& replacements) {
    // JUCE allocates coefficients on the HEAP
    *old = *replacements;
}

Coefficients FilterModuleDSP::makePeakFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq, chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

FilterChainSettings FilterModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber) {
    FilterChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq " + std::to_string(parameterNumber))->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq " + std::to_string(parameterNumber))->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq " + std::to_string(parameterNumber))->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain " + std::to_string(parameterNumber))->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality " + std::to_string(parameterNumber))->load();
    settings.lowCutSlope = apvts.getRawParameterValue("LowCut Slope " + std::to_string(parameterNumber))->load();
    settings.highCutSlope = apvts.getRawParameterValue("HighCut Slope " + std::to_string(parameterNumber))->load();
    // bypass
    settings.bypassed = apvts.getRawParameterValue("Filter Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.analyzerBypassed = apvts.getRawParameterValue("Filter Analyzer Enabled " + std::to_string(parameterNumber))->load() > 0.5f;
    return settings;
}

void FilterModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    for (int i = 1; i < 9; ++i) {
        auto lowCutFreq = std::make_unique<juce::AudioParameterFloat>(
            "LowCut Freq " + std::to_string(i),
            "LowCut Freq " + std::to_string(i),
            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
            20.f,
            "Filter " + std::to_string(i)
            );
        auto highCutFreq = std::make_unique<juce::AudioParameterFloat>(
            "HighCut Freq " + std::to_string(i),
            "HighCut Freq " + std::to_string(i),
            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
            20000.f,
            "Filter " + std::to_string(i)
            );
        auto peakFreq = std::make_unique<juce::AudioParameterFloat>(
            "Peak Freq " + std::to_string(i),
            "Peak Freq " + std::to_string(i),
            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
            800.f,
            "Filter " + std::to_string(i)
            );
        auto peakGain = std::make_unique<juce::AudioParameterFloat>(
            "Peak Gain " + std::to_string(i),
            "Peak Gain " + std::to_string(i),
            juce::NormalisableRange<float>(-24.f, 24.f, 0.5, 1.f),
            0.0f,
            "Filter " + std::to_string(i)
            );
        auto peakQuality = std::make_unique<juce::AudioParameterFloat>(
            "Peak Quality " + std::to_string(i),
            "Peak Quality " + std::to_string(i),
            juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
            1.f,
            "Filter " + std::to_string(i)
            );

        juce::StringArray stringArray;

        for (int i = 0; i < 4; ++i) {
            juce::String string;
            string << (12 + i * 12);
            string << " db/Octave";
            stringArray.add(string);
        }

        auto lowCutSlope = std::make_unique<juce::AudioParameterChoice>(
            "LowCut Slope " + std::to_string(i),
            "LowCut Slope " + std::to_string(i),
            stringArray,
            0,
            "Filter " + std::to_string(i)
            );
        auto highCutSlope = std::make_unique<juce::AudioParameterChoice>(
            "HighCut Slope " + std::to_string(i),
            "HighCut Slope " + std::to_string(i),
            stringArray,
            0,
            "Filter " + std::to_string(i)
            );

        auto lowCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("LowCut " + std::to_string(i), "LowCut " + std::to_string(i),
            "|", std::move(lowCutFreq), std::move(lowCutSlope));
        auto highCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("HighCut " + std::to_string(i), "HighCut " + std::to_string(i),
            "|", std::move(highCutFreq), std::move(highCutSlope));
        auto peakGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Peak " + std::to_string(i), "Peak " + std::to_string(i),
            "|", std::move(peakFreq), std::move(peakQuality), std::move(peakGain));

        layout.add(std::move(lowCutGroup));
        layout.add(std::move(highCutGroup));
        layout.add(std::move(peakGroup));

        layout.add(std::make_unique<AudioParameterBool>("Filter Bypassed " + std::to_string(i), "Filter Bypassed " + std::to_string(i), false, "Filter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("Filter Analyzer Enabled " + std::to_string(i), "Filter Analyzer Enabled " + std::to_string(i), true, "Filter " + std::to_string(i)));
    }

}

MonoChain* FilterModuleDSP::getOneChain()
{
    return &leftChain;
}

void FilterModuleDSP::updateDSPState(double sampleRate) {
    auto settings = getSettings(apvts, parameterNumber);

    bypassed = settings.bypassed;
    updateLowCutFilter(settings, sampleRate);
    updatePeakFilter(settings, sampleRate);
    updateHighCutFilter(settings, sampleRate);
}

void FilterModuleDSP::updatePeakFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto peakCoefficients = makePeakFilter(chainSettings, sampleRate);
    // JUCE allocates coefficients on the HEAP
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    leftChain.setBypassed<ChainPositions::Peak>(chainSettings.bypassed);
    rightChain.setBypassed<ChainPositions::Peak>(chainSettings.bypassed);
}

void FilterModuleDSP::updateLowCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.bypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.bypassed);
}

void FilterModuleDSP::updateHighCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.bypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.bypassed);
}

void FilterModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateDSPState(sampleRate);
}

void FilterModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    updateDSPState(sampleRate);

    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================

/* FilterModule GUI */

//==============================================================================

FilterModuleGUI::FilterModuleGUI(PluginState& p, unsigned int parameterNumber)
    : GUIModule(), pluginState(p),
    peakFreqSlider(*pluginState.apvts.getParameter("Peak Freq " + std::to_string(parameterNumber)), "Hz"),
    peakGainSlider(*pluginState.apvts.getParameter("Peak Gain " + std::to_string(parameterNumber)), "dB"),
    peakQualitySlider(*pluginState.apvts.getParameter("Peak Quality " + std::to_string(parameterNumber)), ""),
    lowCutFreqSlider(*pluginState.apvts.getParameter("LowCut Freq " + std::to_string(parameterNumber)), "Hz"),
    highCutFreqSlider(*pluginState.apvts.getParameter("HighCut Freq " + std::to_string(parameterNumber)), "Hz"),
    lowCutSlopeSlider(*pluginState.apvts.getParameter("LowCut Slope " + std::to_string(parameterNumber)), "dB/Oct"),
    highCutSlopeSlider(*pluginState.apvts.getParameter("HighCut Slope " + std::to_string(parameterNumber)), "dB/Oct"),
    responseCurveComponent(p, parameterNumber),
    filterFftAnalyzerComponent(p, parameterNumber),
    peakFreqSliderAttachment(pluginState.apvts, "Peak Freq " + std::to_string(parameterNumber), peakFreqSlider),
    peakGainSliderAttachment(pluginState.apvts, "Peak Gain " + std::to_string(parameterNumber), peakGainSlider),
    peakQualitySliderAttachment(pluginState.apvts, "Peak Quality " + std::to_string(parameterNumber), peakQualitySlider),
    lowCutFreqSliderAttachment(pluginState.apvts, "LowCut Freq " + std::to_string(parameterNumber), lowCutFreqSlider),
    highCutFreqSliderAttachment(pluginState.apvts, "HighCut Freq " + std::to_string(parameterNumber), highCutFreqSlider),
    lowCutSlopeSliderAttachment(pluginState.apvts, "LowCut Slope " + std::to_string(parameterNumber), lowCutSlopeSlider),
    highCutSlopeSliderAttachment(pluginState.apvts, "HighCut Slope " + std::to_string(parameterNumber), highCutSlopeSlider),
    bypassButtonAttachment(pluginState.apvts, "Filter Bypassed " + std::to_string(parameterNumber), bypassButton),
    analyzerButtonAttachment(pluginState.apvts, "Filter Analyzer Enabled " + std::to_string(parameterNumber), analyzerButton)
{
    // title setup
    title.setText("Filter", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    moduleColor = moduleType_colors.at(ModuleType::IIRFilter);

    // labels

    peakFreqSlider.labels.add({ 0.f, "20Hz" });
    peakFreqSlider.labels.add({ 1.f, "20kHz" });
    peakGainSlider.labels.add({ 0.f, "-24dB" });
    peakGainSlider.labels.add({ 1.f, "+24dB" });
    peakQualitySlider.labels.add({ 0.f, "0.1" });
    peakQualitySlider.labels.add({ 1.f, "10.0" });
    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20kHz" });
    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20kHz" });
    lowCutSlopeSlider.labels.add({ 0.0f, "12" });
    lowCutSlopeSlider.labels.add({ 1.f, "48" });
    highCutSlopeSlider.labels.add({ 0.0f, "12" });
    highCutSlopeSlider.labels.add({ 1.f, "48" });

    // buttons
    lnf.color = moduleType_colors.at(ModuleType::IIRFilter);
    bypassButton.setLookAndFeel(&lnf);
    analyzerButton.setLookAndFeel(&lnf);

    auto safePtr = juce::Component::SafePointer<FilterModuleGUI>(this);
    bypassButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto bypassed = comp->bypassButton.getToggleState();
            comp->handleParamCompsEnablement(bypassed);
        }
    };

    analyzerButton.onClick = [safePtr]()
    {
        if (auto* comp = safePtr.getComponent())
        {
            auto enable = comp->analyzerButton.getToggleState();
            comp->filterFftAnalyzerComponent.toggleFFTanaysis(enable);
        }
    };

    filterFftAnalyzerComponent.toggleFFTanaysis(analyzerButton.getToggleState());

    // tooltips
    bypassButton.setTooltip("Bypass this module");
    analyzerButton.setTooltip("Enable the spectrum analyzer");
    lowCutFreqSlider.setTooltip("Set the lowcut filter frequency");
    lowCutSlopeSlider.setTooltip("Set the lowcut filter slope");
    peakFreqSlider.setTooltip("Set the peak filter frequency");
    peakGainSlider.setTooltip("Set the peak filter gain");
    peakQualitySlider.setTooltip("Set the peak filter quality");
    highCutFreqSlider.setTooltip("Set the highcut filter frequency");
    highCutSlopeSlider.setTooltip("Set the highcut filter slope");

    for (auto* comp : getAllComps())
    {
        addAndMakeVisible(comp);
    }

    handleParamCompsEnablement(bypassButton.getToggleState());
}

FilterModuleGUI::~FilterModuleGUI()
{
    bypassButton.setLookAndFeel(nullptr);
    analyzerButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> FilterModuleGUI::getAllComps()
{
    return {
        &title,
        // filter
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        // responseCurve
        &filterFftAnalyzerComponent,
        &responseCurveComponent,
        // bypass
        &bypassButton,
        &analyzerButton
    };
}

std::vector<juce::Component*> FilterModuleGUI::getParamComps()
{
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
    };
}

void FilterModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();
    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    peakFreqSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    peakGainSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    peakQualitySlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    lowCutFreqSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    highCutFreqSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    lowCutSlopeSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    highCutSlopeSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    analyzerButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
}

void FilterModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto peakFreq = pluginState.apvts.getParameter("Peak Freq " + std::to_string(parameterNumber));
    auto peakGain = pluginState.apvts.getParameter("Peak Gain " + std::to_string(parameterNumber));
    auto peakQuality = pluginState.apvts.getParameter("Peak Quality " + std::to_string(parameterNumber));
    auto lowcutFreq = pluginState.apvts.getParameter("LowCut Freq " + std::to_string(parameterNumber));
    auto highcutFreq = pluginState.apvts.getParameter("HighCut Freq " + std::to_string(parameterNumber));
    auto lowcutSlope = pluginState.apvts.getParameter("LowCut Slope " + std::to_string(parameterNumber));
    auto highcutSlope = pluginState.apvts.getParameter("HighCut Slope " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter("Filter Bypassed " + std::to_string(parameterNumber));
    auto analyzerEnabled = pluginState.apvts.getParameter("Filter Analyzer Enabled " + std::to_string(parameterNumber));

    peakFreq->setValueNotifyingHost(peakFreq->getDefaultValue());
    peakGain->setValueNotifyingHost(peakGain->getDefaultValue());
    peakQuality->setValueNotifyingHost(peakQuality->getDefaultValue());
    lowcutFreq->setValueNotifyingHost(lowcutFreq->getDefaultValue());
    highcutFreq->setValueNotifyingHost(highcutFreq->getDefaultValue());
    lowcutSlope->setValueNotifyingHost(lowcutSlope->getDefaultValue());
    highcutSlope->setValueNotifyingHost(highcutSlope->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
    analyzerEnabled->setValueNotifyingHost(analyzerEnabled->getDefaultValue());
}

juce::Array<juce::var> FilterModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(peakFreqSlider.getValue()));
    values.add(juce::var(peakGainSlider.getValue()));
    values.add(juce::var(peakQualitySlider.getValue()));
    values.add(juce::var(lowCutFreqSlider.getValue()));
    values.add(juce::var(highCutFreqSlider.getValue()));
    values.add(juce::var(lowCutSlopeSlider.getValue()));
    values.add(juce::var(highCutSlopeSlider.getValue()));
    values.add(juce::var(analyzerButton.getToggleState()));

    return values;
}

void FilterModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
    // filter types
    g.setColour(juce::Colours::white);
    g.setFont(ModuleLookAndFeel::getLabelsFont());
    g.drawFittedText("LowCut", lowCutFreqSlider.getBounds().translated(0, -14), juce::Justification::centredTop, 1);
    g.drawFittedText("Peak", peakFreqSlider.getBounds().translated(0, -14), juce::Justification::centredTop, 1);
    g.drawFittedText("HighCut", highCutFreqSlider.getBounds().translated(0, -14), juce::Justification::centredTop, 1);
}

void FilterModuleGUI::resized()
{
    auto filtersArea = getContentRenderArea();

    // bypass
    auto temp = filtersArea;
    auto bypassButtonArea = temp.removeFromTop(25);

    bypassButtonArea.setWidth(35.f);
    bypassButtonArea.setX(128.f);
    bypassButtonArea.setY(20.f);

    bypassButton.setBounds(bypassButtonArea);

    // analyzer button
    auto temp2 = filtersArea;
    auto analyzerButtonArea = temp2.removeFromTop(25);

    analyzerButtonArea.setWidth(50.f);
    analyzerButtonArea.setX(467.f);
    analyzerButtonArea.setY(22.f);

    analyzerButton.setBounds(analyzerButtonArea);

    auto titleAndBypassArea = filtersArea.removeFromTop(30);
    titleAndBypassArea.translate(0, 4);

    auto responseCurveArea = filtersArea.removeFromTop(filtersArea.getHeight() * (1.f / 2.f));
    responseCurveArea.reduce(10, 10);
    auto lowCutArea = filtersArea.removeFromLeft(filtersArea.getWidth() * (1.f / 3.f));
    auto highCutArea = filtersArea.removeFromRight(filtersArea.getWidth() * (1.f / 2.f));
    auto lfArea = lowCutArea.removeFromTop(lowCutArea.getHeight() * (1.f / 2.f));
    auto lsArea = lowCutArea;
    auto hfArea = highCutArea.removeFromTop(highCutArea.getHeight() * (1.f / 2.f));
    auto hsArea = highCutArea;
    auto pfArea = filtersArea.removeFromTop(filtersArea.getHeight() * 0.33);
    auto pgArea = filtersArea.removeFromTop(filtersArea.getHeight() * 0.5);
    auto pqArea = filtersArea;

    juce::Rectangle<int> renderArea;
    renderArea.setSize(lfArea.getHeight(), lfArea.getHeight());
    const int offset = 6;

    // title

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centredBottom);

    // fft analyzer & response curve

    filterFftAnalyzerComponent.setBounds(responseCurveArea);
    responseCurveComponent.setBounds(responseCurveArea);

    // lowpass

    renderArea.setCentre(lfArea.getCentre());
    renderArea.setY(lfArea.getTopLeft().getY() + offset);
    lowCutFreqSlider.setBounds(renderArea);

    renderArea.setCentre(lsArea.getCentre());
    renderArea.setY(lsArea.getTopLeft().getY() + offset);
    lowCutSlopeSlider.setBounds(renderArea);

    // highpass

    renderArea.setCentre(hfArea.getCentre());
    renderArea.setY(hfArea.getTopLeft().getY() + offset);
    highCutFreqSlider.setBounds(renderArea);

    renderArea.setCentre(hsArea.getCentre());
    renderArea.setY(hsArea.getTopLeft().getY() + offset);
    highCutSlopeSlider.setBounds(renderArea);

    // peak

    renderArea.setSize(pfArea.getHeight() + 10, pfArea.getHeight());

    renderArea.setCentre(pfArea.getCentre());
    renderArea.setY(pfArea.getTopLeft().getY() + offset);
    peakFreqSlider.setBounds(renderArea);

    renderArea.setCentre(pgArea.getCentre());
    renderArea.setY(pgArea.getTopLeft().getY() + offset);
    peakGainSlider.setBounds(renderArea);

    renderArea.setCentre(pqArea.getCentre());
    renderArea.setY(pqArea.getTopLeft().getY() + offset);
    peakQualitySlider.setBounds(renderArea);
}