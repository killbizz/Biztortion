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

Coefficients FilterModuleDSP::makePeakFilter(const ChainPositions& chainPosition, const FilterChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate,
        chainPosition == ChainPositions::Peak1 ? chainSettings.peak1Freq : chainSettings.peak2Freq,
        chainPosition == ChainPositions::Peak1 ? chainSettings.peak1Quality : chainSettings.peak2Quality,
        chainPosition == ChainPositions::Peak1 ? juce::Decibels::decibelsToGain(chainSettings.peak1GainInDecibels) : juce::Decibels::decibelsToGain(chainSettings.peak2GainInDecibels)
    );
}

FilterChainSettings FilterModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber) {
    FilterChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq " + std::to_string(parameterNumber))->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq " + std::to_string(parameterNumber))->load();
    // PEAK1
    settings.peak1Freq = apvts.getRawParameterValue("Peak1 Freq " + std::to_string(parameterNumber))->load();
    settings.peak1GainInDecibels = apvts.getRawParameterValue("Peak1 Gain " + std::to_string(parameterNumber))->load();
    settings.peak1Quality = apvts.getRawParameterValue("Peak1 Quality " + std::to_string(parameterNumber))->load();
    // PEAK2
    settings.peak2Freq = apvts.getRawParameterValue("Peak2 Freq " + std::to_string(parameterNumber))->load();
    settings.peak2GainInDecibels = apvts.getRawParameterValue("Peak2 Gain " + std::to_string(parameterNumber))->load();
    settings.peak2Quality = apvts.getRawParameterValue("Peak2 Quality " + std::to_string(parameterNumber))->load();
    settings.lowCutSlope = apvts.getRawParameterValue("LowCut Slope " + std::to_string(parameterNumber))->load();
    settings.highCutSlope = apvts.getRawParameterValue("HighCut Slope " + std::to_string(parameterNumber))->load();
    // bypass
    settings.bypassed = apvts.getRawParameterValue("Filter Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.peak1Bypassed = apvts.getRawParameterValue("Peak1 Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.peak2Bypassed = apvts.getRawParameterValue("Peak2 Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;
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
        // PEAK 1
        auto peak1Freq = std::make_unique<juce::AudioParameterFloat>(
            "Peak1 Freq " + std::to_string(i),
            "Peak1 Freq " + std::to_string(i),
            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
            330.f,
            "Filter " + std::to_string(i)
            );
        auto peak1Gain = std::make_unique<juce::AudioParameterFloat>(
            "Peak1 Gain " + std::to_string(i),
            "Peak1 Gain " + std::to_string(i),
            juce::NormalisableRange<float>(-24.f, 24.f, 0.5, 1.f),
            0.0f,
            "Filter " + std::to_string(i)
            );
        auto peak1Quality = std::make_unique<juce::AudioParameterFloat>(
            "Peak1 Quality " + std::to_string(i),
            "Peak1 Quality " + std::to_string(i),
            juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
            1.f,
            "Filter " + std::to_string(i)
            );
        // PEAK 2
        auto peak2Freq = std::make_unique<juce::AudioParameterFloat>(
            "Peak2 Freq " + std::to_string(i),
            "Peak2 Freq " + std::to_string(i),
            juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
            1240.f,
            "Filter " + std::to_string(i)
            );
        auto peak2Gain = std::make_unique<juce::AudioParameterFloat>(
            "Peak2 Gain " + std::to_string(i),
            "Peak2 Gain " + std::to_string(i),
            juce::NormalisableRange<float>(-24.f, 24.f, 0.5, 1.f),
            0.0f,
            "Filter " + std::to_string(i)
            );
        auto peak2Quality = std::make_unique<juce::AudioParameterFloat>(
            "Peak2 Quality " + std::to_string(i),
            "Peak2 Quality " + std::to_string(i),
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
        auto peak1Group = std::make_unique<juce::AudioProcessorParameterGroup>("Peak1 " + std::to_string(i), "Peak1 " + std::to_string(i),
            "|", std::move(peak1Freq), std::move(peak1Quality), std::move(peak1Gain));
        auto peak2Group = std::make_unique<juce::AudioProcessorParameterGroup>("Peak2 " + std::to_string(i), "Peak2 " + std::to_string(i),
            "|", std::move(peak2Freq), std::move(peak2Quality), std::move(peak2Gain));

        layout.add(std::move(lowCutGroup));
        layout.add(std::move(highCutGroup));
        layout.add(std::move(peak1Group));
        layout.add(std::move(peak2Group));

        layout.add(std::make_unique<AudioParameterBool>("LowCut Bypassed " + std::to_string(i), "LowCut Bypassed " + std::to_string(i), true, "Filter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("Peak1 Bypassed " + std::to_string(i), "Peak1 Bypassed " + std::to_string(i), false, "Filter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("Peak2 Bypassed " + std::to_string(i), "Peak2 Bypassed " + std::to_string(i), false, "Filter " + std::to_string(i)));
        layout.add(std::make_unique<AudioParameterBool>("HighCut Bypassed " + std::to_string(i), "HighCut Bypassed " + std::to_string(i), true, "Filter " + std::to_string(i)));

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
    updatePeakFilters(settings, sampleRate);
    updateHighCutFilter(settings, sampleRate);
}

void FilterModuleDSP::updatePeakFilters(const FilterChainSettings& chainSettings, double sampleRate) {    
    // PEAK1
    auto peak1Coefficients = makePeakFilter(ChainPositions::Peak1, chainSettings, sampleRate);
    updateCoefficients(leftChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    // PEAK2
    auto peak2Coefficients = makePeakFilter(ChainPositions::Peak2, chainSettings, sampleRate);
    updateCoefficients(leftChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);

    leftChain.setBypassed<ChainPositions::Peak1>(chainSettings.peak1Bypassed || chainSettings.bypassed);
    rightChain.setBypassed<ChainPositions::Peak2>(chainSettings.peak2Bypassed || chainSettings.bypassed);
}

void FilterModuleDSP::updateLowCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed || chainSettings.bypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed || chainSettings.bypassed);
}

void FilterModuleDSP::updateHighCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed || chainSettings.bypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed || chainSettings.bypassed);
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
    peak1FreqSlider(*pluginState.apvts.getParameter("Peak1 Freq " + std::to_string(parameterNumber)), "Hz"),
    peak1GainSlider(*pluginState.apvts.getParameter("Peak1 Gain " + std::to_string(parameterNumber)), "dB"),
    peak1QualitySlider(*pluginState.apvts.getParameter("Peak1 Quality " + std::to_string(parameterNumber)), ""),
    peak2FreqSlider(*pluginState.apvts.getParameter("Peak2 Freq " + std::to_string(parameterNumber)), "Hz"),
    peak2GainSlider(*pluginState.apvts.getParameter("Peak2 Gain " + std::to_string(parameterNumber)), "dB"),
    peak2QualitySlider(*pluginState.apvts.getParameter("Peak2 Quality " + std::to_string(parameterNumber)), ""),
    lowCutFreqSlider(*pluginState.apvts.getParameter("LowCut Freq " + std::to_string(parameterNumber)), "Hz"),
    highCutFreqSlider(*pluginState.apvts.getParameter("HighCut Freq " + std::to_string(parameterNumber)), "Hz"),
    lowCutSlopeSlider(*pluginState.apvts.getParameter("LowCut Slope " + std::to_string(parameterNumber)), "dB/Oct"),
    highCutSlopeSlider(*pluginState.apvts.getParameter("HighCut Slope " + std::to_string(parameterNumber)), "dB/Oct"),
    responseCurveComponent(p, parameterNumber),
    filterFftAnalyzerComponent(p, parameterNumber),
    peak1FreqSliderAttachment(pluginState.apvts, "Peak1 Freq " + std::to_string(parameterNumber), peak1FreqSlider),
    peak1GainSliderAttachment(pluginState.apvts, "Peak1 Gain " + std::to_string(parameterNumber), peak1GainSlider),
    peak1QualitySliderAttachment(pluginState.apvts, "Peak1 Quality " + std::to_string(parameterNumber), peak1QualitySlider),
    peak2FreqSliderAttachment(pluginState.apvts, "Peak2 Freq " + std::to_string(parameterNumber), peak2FreqSlider),
    peak2GainSliderAttachment(pluginState.apvts, "Peak2 Gain " + std::to_string(parameterNumber), peak2GainSlider),
    peak2QualitySliderAttachment(pluginState.apvts, "Peak2 Quality " + std::to_string(parameterNumber), peak2QualitySlider),
    lowCutFreqSliderAttachment(pluginState.apvts, "LowCut Freq " + std::to_string(parameterNumber), lowCutFreqSlider),
    highCutFreqSliderAttachment(pluginState.apvts, "HighCut Freq " + std::to_string(parameterNumber), highCutFreqSlider),
    lowCutSlopeSliderAttachment(pluginState.apvts, "LowCut Slope " + std::to_string(parameterNumber), lowCutSlopeSlider),
    highCutSlopeSliderAttachment(pluginState.apvts, "HighCut Slope " + std::to_string(parameterNumber), highCutSlopeSlider),
    bypassButtonAttachment(pluginState.apvts, "Filter Bypassed " + std::to_string(parameterNumber), bypassButton),
    lowCutBypassButtonAttachment(pluginState.apvts, "LowCut Bypassed " + std::to_string(parameterNumber), lowCutBypassButton),
    peak1BypassButtonAttachment(pluginState.apvts, "Peak1 Bypassed " + std::to_string(parameterNumber), peak1BypassButton),
    peak2BypassButtonAttachment(pluginState.apvts, "Peak2 Bypassed " + std::to_string(parameterNumber), peak2BypassButton),
    highCutBypassButtonAttachment(pluginState.apvts, "HighCut Bypassed " + std::to_string(parameterNumber), highCutBypassButton),
    analyzerButtonAttachment(pluginState.apvts, "Filter Analyzer Enabled " + std::to_string(parameterNumber), analyzerButton)
{
    // title setup
    title.setText("Filter", juce::dontSendNotification);
    title.setFont(ModuleLookAndFeel::getTitlesFont());

    moduleColor = moduleType_colors.at(ModuleType::IIRFilter);

    // labels
    // peak1
    peak1FreqSlider.labels.add({ 0.f, "20Hz" });
    peak1FreqSlider.labels.add({ 1.f, "20kHz" });
    peak1GainSlider.labels.add({ 0.f, "-24dB" });
    peak1GainSlider.labels.add({ 1.f, "+24dB" });
    peak1QualitySlider.labels.add({ 0.f, "0.1" });
    peak1QualitySlider.labels.add({ 1.f, "10.0" });
    // peak2
    peak2FreqSlider.labels.add({ 0.f, "20Hz" });
    peak2FreqSlider.labels.add({ 1.f, "20kHz" });
    peak2GainSlider.labels.add({ 0.f, "-24dB" });
    peak2GainSlider.labels.add({ 1.f, "+24dB" });
    peak2QualitySlider.labels.add({ 0.f, "0.1" });
    peak2QualitySlider.labels.add({ 1.f, "10.0" });
    // cut filters
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
    lowCutBypassButton.setLookAndFeel(&lnf);
    peak1BypassButton.setLookAndFeel(&lnf);
    peak2BypassButton.setLookAndFeel(&lnf);
    highCutBypassButton.setLookAndFeel(&lnf);
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
    lowCutBypassButton.setTooltip("Bypass the lowcut filter");
    highCutBypassButton.setTooltip("Bypass the highcut filter");
    peak1BypassButton.setTooltip("Bypass the first peak filter");
    peak2BypassButton.setTooltip("Bypass the second peak filter");
    analyzerButton.setTooltip("Enable the spectrum analyzer");
    lowCutFreqSlider.setTooltip("Set the lowcut filter frequency");
    lowCutSlopeSlider.setTooltip("Set the lowcut filter slope");
    peak1FreqSlider.setTooltip("Set the peak filter frequency");
    peak1GainSlider.setTooltip("Set the peak filter gain");
    peak1QualitySlider.setTooltip("Set the peak filter quality");
    peak2FreqSlider.setTooltip("Set the peak filter frequency");
    peak2GainSlider.setTooltip("Set the peak filter gain");
    peak2QualitySlider.setTooltip("Set the peak filter quality");
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
    lowCutBypassButton.setLookAndFeel(nullptr);
    highCutBypassButton.setLookAndFeel(nullptr);
    peak1BypassButton.setLookAndFeel(nullptr);
    peak2BypassButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> FilterModuleGUI::getAllComps()
{
    return {
        &title,
        // filter
        &peak1FreqSlider,
        &peak1GainSlider,
        &peak1QualitySlider,
        &peak2FreqSlider,
        &peak2GainSlider,
        &peak2QualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        // responseCurve
        &filterFftAnalyzerComponent,
        &responseCurveComponent,
        // bypass
        &bypassButton,
        &analyzerButton,
        &lowCutBypassButton,
        &highCutBypassButton,
        &peak1BypassButton,
        &peak2BypassButton
    };
}

std::vector<juce::Component*> FilterModuleGUI::getParamComps()
{
    return {
        &peak1FreqSlider,
        &peak1GainSlider,
        &peak1QualitySlider,
        &peak2FreqSlider,
        &peak2GainSlider,
        &peak2QualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &lowCutBypassButton,
        &highCutBypassButton,
        &peak1BypassButton,
        &peak2BypassButton
    };
}

void FilterModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
    auto value = values.begin();
    
    peak1FreqSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    peak1GainSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    peak1QualitySlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    peak2FreqSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    peak2GainSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    peak2QualitySlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    lowCutFreqSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    highCutFreqSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    lowCutSlopeSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    highCutSlopeSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
    bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    analyzerButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    lowCutBypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    peak1BypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    peak2BypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
    highCutBypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
}

void FilterModuleGUI::resetParameters(unsigned int parameterNumber)
{
    auto peak1Freq = pluginState.apvts.getParameter("Peak1 Freq " + std::to_string(parameterNumber));
    auto peak1Gain = pluginState.apvts.getParameter("Peak1 Gain " + std::to_string(parameterNumber));
    auto peak1Quality = pluginState.apvts.getParameter("Peak1 Quality " + std::to_string(parameterNumber));
    auto peak2Freq = pluginState.apvts.getParameter("Peak2 Freq " + std::to_string(parameterNumber));
    auto peak2Gain = pluginState.apvts.getParameter("Peak2 Gain " + std::to_string(parameterNumber));
    auto peak2Quality = pluginState.apvts.getParameter("Peak2 Quality " + std::to_string(parameterNumber));
    auto lowcutFreq = pluginState.apvts.getParameter("LowCut Freq " + std::to_string(parameterNumber));
    auto highcutFreq = pluginState.apvts.getParameter("HighCut Freq " + std::to_string(parameterNumber));
    auto lowcutSlope = pluginState.apvts.getParameter("LowCut Slope " + std::to_string(parameterNumber));
    auto highcutSlope = pluginState.apvts.getParameter("HighCut Slope " + std::to_string(parameterNumber));
    auto bypassed = pluginState.apvts.getParameter("Filter Bypassed " + std::to_string(parameterNumber));
    auto analyzerEnabled = pluginState.apvts.getParameter("Filter Analyzer Enabled " + std::to_string(parameterNumber));
    auto lowCutBypassed = pluginState.apvts.getParameter("LowCut Bypassed " + std::to_string(parameterNumber));
    auto peak1Bypassed = pluginState.apvts.getParameter("Peak1 Bypassed " + std::to_string(parameterNumber));
    auto peak2Bypassed = pluginState.apvts.getParameter("Peak2 Bypassed " + std::to_string(parameterNumber));
    auto highCutBypassed = pluginState.apvts.getParameter("HighCut Bypassed " + std::to_string(parameterNumber));

    peak1Freq->setValueNotifyingHost(peak1Freq->getDefaultValue());
    peak1Gain->setValueNotifyingHost(peak1Gain->getDefaultValue());
    peak1Quality->setValueNotifyingHost(peak1Quality->getDefaultValue());
    peak2Freq->setValueNotifyingHost(peak2Freq->getDefaultValue());
    peak2Gain->setValueNotifyingHost(peak2Gain->getDefaultValue());
    peak2Quality->setValueNotifyingHost(peak2Quality->getDefaultValue());
    lowcutFreq->setValueNotifyingHost(lowcutFreq->getDefaultValue());
    highcutFreq->setValueNotifyingHost(highcutFreq->getDefaultValue());
    lowcutSlope->setValueNotifyingHost(lowcutSlope->getDefaultValue());
    highcutSlope->setValueNotifyingHost(highcutSlope->getDefaultValue());
    bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
    analyzerEnabled->setValueNotifyingHost(analyzerEnabled->getDefaultValue());
    lowCutBypassed->setValueNotifyingHost(lowCutBypassed->getDefaultValue());
    highCutBypassed->setValueNotifyingHost(highCutBypassed->getDefaultValue());
    peak1Bypassed->setValueNotifyingHost(peak1Bypassed->getDefaultValue());
    peak2Bypassed->setValueNotifyingHost(peak2Bypassed->getDefaultValue());
}

juce::Array<juce::var> FilterModuleGUI::getParamValues()
{
    juce::Array<juce::var> values;

    values.add(juce::var(peak1FreqSlider.getValue()));
    values.add(juce::var(peak1GainSlider.getValue()));
    values.add(juce::var(peak1QualitySlider.getValue()));
    values.add(juce::var(peak2FreqSlider.getValue()));
    values.add(juce::var(peak2GainSlider.getValue()));
    values.add(juce::var(peak2QualitySlider.getValue()));
    values.add(juce::var(lowCutFreqSlider.getValue()));
    values.add(juce::var(highCutFreqSlider.getValue()));
    values.add(juce::var(lowCutSlopeSlider.getValue()));
    values.add(juce::var(highCutSlopeSlider.getValue()));
    values.add(juce::var(bypassButton.getToggleState()));
    values.add(juce::var(analyzerButton.getToggleState()));
    values.add(juce::var(lowCutBypassButton.getToggleState()));
    values.add(juce::var(peak1BypassButton.getToggleState()));
    values.add(juce::var(peak2BypassButton.getToggleState()));
    values.add(juce::var(highCutBypassButton.getToggleState()));

    return values;
}

void FilterModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
    // filter types
    g.setColour(juce::Colours::white);
    g.setFont(ModuleLookAndFeel::getLabelsFont());
    g.drawFittedText("LowCut", lowCutFreqSlider.getBounds().translated(0, -14), juce::Justification::centredTop, 1);
    g.drawFittedText("Peak", peak1FreqSlider.getBounds().translated(0, -14), juce::Justification::centredTop, 1);
    g.drawFittedText("Peak", peak2FreqSlider.getBounds().translated(0, -14), juce::Justification::centredTop, 1);
    g.drawFittedText("HighCut", highCutFreqSlider.getBounds().translated(0, -14), juce::Justification::centredTop, 1);
}

void FilterModuleGUI::resized()
{
    auto filtersArea = getContentRenderArea();

    // bypass
    auto temp = filtersArea;
    auto bypassButtonArea = temp.removeFromTop(25);

    auto size = 32.f;
    bypassButtonArea.setSize(size, size);
    bypassButton.setBounds(bypassButtonArea);
    bypassButton.setCentreRelative(0.23f,0.088f);

    // analyzer button
    auto temp2 = filtersArea;
    auto analyzerButtonArea = temp2.removeFromTop(25);

    analyzerButtonArea.setWidth(50.f);
    analyzerButtonArea.setX(466.626f);
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
    // peak filters
    auto leftPeakArea = filtersArea.removeFromLeft(filtersArea.getWidth() * (1.f / 2.f));
    auto rightPeakArea = filtersArea;
    auto leftPeakFreqArea = leftPeakArea.removeFromTop(leftPeakArea.getHeight() * 0.33);
    auto leftPeakGainArea = leftPeakArea.removeFromTop(leftPeakArea.getHeight() * 0.5);
    auto leftPeakQualityArea = leftPeakArea;
    auto rightPeakFreqArea = rightPeakArea.removeFromTop(rightPeakArea.getHeight() * 0.33);
    auto rightPeakGainArea = rightPeakArea.removeFromTop(rightPeakArea.getHeight() * 0.5);
    auto rightPeakQualityArea = rightPeakArea;
    // buttons
    auto lowCutButtonArea = bypassButtonArea;
    lowCutButtonArea.setWidth(21);
    lowCutButtonArea.setX(lfArea.getX() + 50);
    lowCutButtonArea.setY(lfArea.getY() - 13);
    auto peak1ButtonArea = bypassButtonArea;
    peak1ButtonArea.setWidth(21);
    peak1ButtonArea.setX(leftPeakFreqArea.getX() + 8);
    peak1ButtonArea.setY(leftPeakFreqArea.getY() - 13);
    auto peak2ButtonArea = bypassButtonArea;
    peak2ButtonArea.setWidth(21);
    peak2ButtonArea.setX(rightPeakFreqArea.getX() + 69);
    peak2ButtonArea.setY(rightPeakFreqArea.getY() - 13);
    auto highCutButtonArea = bypassButtonArea;
    highCutButtonArea.setWidth(21.f);
    highCutButtonArea.setX(hfArea.getX() + 132);
    highCutButtonArea.setY(hfArea.getY() - 13);

    lowCutBypassButton.setBounds(lowCutButtonArea);
    peak1BypassButton.setBounds(peak1ButtonArea);
    peak2BypassButton.setBounds(peak2ButtonArea);
    highCutBypassButton.setBounds(highCutButtonArea);

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

    // peaks

    renderArea.setSize(leftPeakFreqArea.getHeight() + 10, leftPeakFreqArea.getHeight());

    renderArea.setCentre(leftPeakFreqArea.getCentre());
    renderArea.setY(leftPeakFreqArea.getTopLeft().getY() + offset);
    peak1FreqSlider.setBounds(renderArea);

    renderArea.setCentre(leftPeakGainArea.getCentre());
    renderArea.setY(leftPeakGainArea.getTopLeft().getY() + offset);
    peak1GainSlider.setBounds(renderArea);

    renderArea.setCentre(leftPeakQualityArea.getCentre());
    renderArea.setY(leftPeakQualityArea.getTopLeft().getY() + offset);
    peak1QualitySlider.setBounds(renderArea);

    renderArea.setCentre(rightPeakFreqArea.getCentre());
    renderArea.setY(rightPeakFreqArea.getTopLeft().getY() + offset);
    peak2FreqSlider.setBounds(renderArea);

    renderArea.setCentre(rightPeakGainArea.getCentre());
    renderArea.setY(rightPeakGainArea.getTopLeft().getY() + offset);
    peak2GainSlider.setBounds(renderArea);

    renderArea.setCentre(rightPeakQualityArea.getCentre());
    renderArea.setY(rightPeakQualityArea.getTopLeft().getY() + offset);
    peak2QualitySlider.setBounds(renderArea);
}