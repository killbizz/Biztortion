/*
  ==============================================================================

    FilterModule.cpp
    Created: 17 Aug 2021 4:12:53pm
    Author:  gabri

  ==============================================================================
*/

#include "FilterModule.h"

FilterModule::FilterModule(juce::AudioProcessorValueTreeState& _apvts)
    : apvts(_apvts) {}

void updateCoefficients(Coefficients& old, const Coefficients& replacements) {
    // JUCE allocates coefficients on the HEAP
    *old = *replacements;
}

Coefficients FilterModule::makePeakFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq, chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

FilterChainSettings FilterModule::getFilterChainSettings(juce::AudioProcessorValueTreeState& apvts) {
    FilterChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = apvts.getRawParameterValue("LowCut Slope")->load();
    settings.highCutSlope = apvts.getRawParameterValue("HighCut Slope")->load();

    return settings;
}

void FilterModule::addFilterParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    auto lowCutFreq = std::make_unique<juce::AudioParameterFloat>(
        "LowCut Freq",
        "LowCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f,
        "Filter"
        );
    auto highCutFreq = std::make_unique<juce::AudioParameterFloat>(
        "HighCut Freq",
        "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f,
        "Filter"
        );
    auto peakFreq = std::make_unique<juce::AudioParameterFloat>(
        "Peak Freq",
        "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        800.f,
        "Filter"
        );
    auto peakGain = std::make_unique<juce::AudioParameterFloat>(
        "Peak Gain",
        "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5, 1.f),
        0.0f,
        "Filter"
        );
    auto peakQuality = std::make_unique<juce::AudioParameterFloat>(
        "Peak Quality",
        "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f,
        "Filter"
        );

    juce::StringArray stringArray;

    for (int i = 0; i < 4; ++i) {
        juce::String string;
        string << (12 + i * 12);
        string << " db/Octave";
        stringArray.add(string);
    }

    auto lowCutSlope = std::make_unique<juce::AudioParameterChoice>(
        "LowCut Slope",
        "LowCut Slope",
        stringArray,
        0,
        "Filter"
        );
    auto highCutSlope = std::make_unique<juce::AudioParameterChoice>(
        "HighCut Slope",
        "HighCut Slope",
        stringArray,
        0,
        "Filter"
        );

    auto lowCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("LowCut", "LowCut", 
        "|", std::move(lowCutFreq), std::move(lowCutSlope));
    auto highCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("HighCut", "HighCut", 
        "|", std::move(highCutFreq), std::move(highCutSlope));
    auto peakGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Peak", "Peak",
        "|", std::move(peakFreq), std::move(peakQuality), std::move(peakGain));

    layout.add(std::move(lowCutGroup));
    layout.add(std::move(highCutGroup));
    layout.add(std::move(peakGroup));
}

void FilterModule::updateFilters(double sampleRate) {
    auto chainSettings = getFilterChainSettings(apvts);
    updateLowCutFilter(chainSettings, sampleRate);
    updatePeakFilter(chainSettings, sampleRate);
    updateHighCutFilter(chainSettings, sampleRate);
}

void FilterModule::updatePeakFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto peakCoefficients = makePeakFilter(chainSettings, sampleRate);
    // JUCE allocates coefficients on the HEAP
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void FilterModule::updateLowCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));
}

void FilterModule::updateHighCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));
}

void FilterModule::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters(sampleRate);
}

void FilterModule::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{
    updateFilters(sampleRate);

    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);
}
