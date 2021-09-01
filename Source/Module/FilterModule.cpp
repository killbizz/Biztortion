/*
  ==============================================================================

    FilterModule.cpp
    Created: 17 Aug 2021 4:12:53pm
    Author:  gabri

  ==============================================================================
*/

#include "FilterModule.h"
#include "../PluginProcessor.h"

//==============================================================================

/* FilterModule DSP */

//==============================================================================

FilterModuleDSP::FilterModuleDSP(juce::AudioProcessorValueTreeState& _apvts, juce::String _type)
    : DSPModule(_apvts), type(_type) {}

void updateCoefficients(Coefficients& old, const Coefficients& replacements) {
    // JUCE allocates coefficients on the HEAP
    *old = *replacements;
}

Coefficients FilterModuleDSP::makePeakFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq, chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

FilterChainSettings FilterModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, juce::String type) {
    FilterChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue(type+" LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue(type + " HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue(type + " Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue(type + " Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue(type + " Peak Quality")->load();
    settings.lowCutSlope = apvts.getRawParameterValue(type + " LowCut Slope")->load();
    settings.highCutSlope = apvts.getRawParameterValue(type + " HighCut Slope")->load();
}

void FilterModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
    // PRE-FILTER

    auto lowCutFreq = std::make_unique<juce::AudioParameterFloat>(
        "Pre LowCut Freq",
        "Pre LowCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f,
        "Pre Filter"
        );
    auto highCutFreq = std::make_unique<juce::AudioParameterFloat>(
        "Pre HighCut Freq",
        "Pre HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f,
        "Pre Filter"
        );
    auto peakFreq = std::make_unique<juce::AudioParameterFloat>(
        "Pre Peak Freq",
        "Pre Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        800.f,
        "Pre Filter"
        );
    auto peakGain = std::make_unique<juce::AudioParameterFloat>(
        "Pre Peak Gain",
        "Pre Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5, 1.f),
        0.0f,
        "Pre Filter"
        );
    auto peakQuality = std::make_unique<juce::AudioParameterFloat>(
        "Pre Peak Quality",
        "Pre Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f,
        "Pre Filter"
        );

    juce::StringArray stringArray;

    for (int i = 0; i < 4; ++i) {
        juce::String string;
        string << (12 + i * 12);
        string << " db/Octave";
        stringArray.add(string);
    }

    auto lowCutSlope = std::make_unique<juce::AudioParameterChoice>(
        "Pre LowCut Slope",
        "Pre LowCut Slope",
        stringArray,
        0,
        "Pre Filter"
        );
    auto highCutSlope = std::make_unique<juce::AudioParameterChoice>(
        "Pre HighCut Slope",
        "Pre HighCut Slope",
        stringArray,
        0,
        "Pre Filter"
        );

    auto lowCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Pre LowCut", "Pre LowCut", 
        "|", std::move(lowCutFreq), std::move(lowCutSlope));
    auto highCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Pre HighCut", "Pre HighCut", 
        "|", std::move(highCutFreq), std::move(highCutSlope));
    auto peakGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Pre Peak", "Pre Peak",
        "|", std::move(peakFreq), std::move(peakQuality), std::move(peakGain));

    // POST-FILTER

    lowCutFreq = std::make_unique<juce::AudioParameterFloat>(
        "Post LowCut Freq",
        "Post LowCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20.f,
        "Post Filter"
        );
    highCutFreq = std::make_unique<juce::AudioParameterFloat>(
        "Post HighCut Freq",
        "Post HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        20000.f,
        "Post Filter"
        );
    peakFreq = std::make_unique<juce::AudioParameterFloat>(
        "Post Peak Freq",
        "Post Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
        800.f,
        "Post Filter"
        );
    peakGain = std::make_unique<juce::AudioParameterFloat>(
        "Post Peak Gain",
        "Post Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5, 1.f),
        0.0f,
        "Post Filter"
        );
    peakQuality = std::make_unique<juce::AudioParameterFloat>(
        "Post Peak Quality",
        "Post Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.f,
        "Post Filter"
        );

    stringArray.clear();

    for (int i = 0; i < 4; ++i) {
        juce::String string;
        string << (12 + i * 12);
        string << " db/Octave";
        stringArray.add(string);
    }

    lowCutSlope = std::make_unique<juce::AudioParameterChoice>(
        "Post LowCut Slope",
        "Post LowCut Slope",
        stringArray,
        0,
        "Post Filter"
        );
    highCutSlope = std::make_unique<juce::AudioParameterChoice>(
        "Post HighCut Slope",
        "Post HighCut Slope",
        stringArray,
        0,
        "Post Filter"
        );

    lowCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Post LowCut", "Post LowCut",
        "|", std::move(lowCutFreq), std::move(lowCutSlope));
    highCutGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Post HighCut", "Post HighCut",
        "|", std::move(highCutFreq), std::move(highCutSlope));
    peakGroup = std::make_unique<juce::AudioProcessorParameterGroup>("Post Peak", "Post Peak",
        "|", std::move(peakFreq), std::move(peakQuality), std::move(peakGain));

    layout.add(std::move(lowCutGroup));
    layout.add(std::move(highCutGroup));
    layout.add(std::move(peakGroup));
}

void FilterModuleDSP::updateDSPState(double sampleRate) {
    auto settings = getSettings(apvts, type);
    updateLowCutFilter(settings, sampleRate);
    updatePeakFilter(settings, sampleRate);
    updateHighCutFilter(settings, sampleRate);
}

void FilterModuleDSP::updatePeakFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto peakCoefficients = makePeakFilter(chainSettings, sampleRate);
    // JUCE allocates coefficients on the HEAP
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void FilterModuleDSP::updateLowCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, sampleRate);
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, static_cast<FilterSlope>(chainSettings.lowCutSlope));
}

void FilterModuleDSP::updateHighCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
    auto highCutCoefficients = makeHighCutFilter(chainSettings, sampleRate);
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, static_cast<FilterSlope>(chainSettings.highCutSlope));
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

/* FilterModule DSP */

//==============================================================================

FilterModuleGUI::FilterModuleGUI(BiztortionAudioProcessor& p, juce::String _type)
    : audioProcessor(p), type(_type),
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
    responseCurveComponent(p, type),
    filterFftAnalyzerComponent(p),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
{
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

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
}

void FilterModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);

    g.setColour(juce::Colours::grey);
    g.setFont(10);
    g.drawFittedText("LowCut", lowCutSlopeSlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("Peak", peakQualitySlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("HighCut", highCutSlopeSlider.getBounds(), juce::Justification::centredBottom, 1);
}

void FilterModuleGUI::resized()
{
    auto bounds = getContentRenderArea();

    auto filtersArea = bounds;
    auto responseCurveArea = filtersArea.removeFromTop(filtersArea.getHeight() * (1.f / 2.f));
    auto lowCutArea = filtersArea.removeFromLeft(filtersArea.getWidth() * (1.f / 3.f));
    auto highCutArea = filtersArea.removeFromRight(filtersArea.getWidth() * (1.f / 2.f));

    filterFftAnalyzerComponent.setBounds(responseCurveArea);
    responseCurveComponent.setBounds(responseCurveArea);
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * (1.f / 2.f)));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * (1.f / 2.f)));
    peakFreqSlider.setBounds(filtersArea.removeFromTop(filtersArea.getHeight() * 0.33));
    peakGainSlider.setBounds(filtersArea.removeFromTop(filtersArea.getHeight() * 0.5));
    peakQualitySlider.setBounds(filtersArea);
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutSlopeSlider.setBounds(highCutArea);
}

std::vector<juce::Component*> FilterModuleGUI::getComps()
{
    return {
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
    };
}
