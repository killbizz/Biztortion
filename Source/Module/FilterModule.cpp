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

FilterChainSettings FilterModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int chainPosition) {
    FilterChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq " + std::to_string(chainPosition))->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq " + std::to_string(chainPosition))->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq " + std::to_string(chainPosition))->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain " + std::to_string(chainPosition))->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality " + std::to_string(chainPosition))->load();
    settings.lowCutSlope = apvts.getRawParameterValue("LowCut Slope " + std::to_string(chainPosition))->load();
    settings.highCutSlope = apvts.getRawParameterValue("HighCut Slope " + std::to_string(chainPosition))->load();

    return settings;
}

void FilterModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{

    // PRE-FILTER

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
    }

}

MonoChain* FilterModuleDSP::getOneChain()
{
    return &leftChain;
}

void FilterModuleDSP::setModuleType()
{
    moduleType = ModuleType::IIRFilter;
}

void FilterModuleDSP::updateDSPState(double sampleRate) {
    auto settings = getSettings(apvts, getChainPosition());
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

/* FilterModule GUI */

//==============================================================================

FilterModuleGUI::FilterModuleGUI(BiztortionAudioProcessor& p, unsigned int chainPosition)
    : GUIModule(), audioProcessor(p),
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq " + std::to_string(chainPosition)), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain " + std::to_string(chainPosition)), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality " + std::to_string(chainPosition)), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq " + std::to_string(chainPosition)), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq " + std::to_string(chainPosition)), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope " + std::to_string(chainPosition)), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope " + std::to_string(chainPosition)), "dB/Oct"),
    responseCurveComponent(p, chainPosition),
    filterFftAnalyzerComponent(p, chainPosition),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq " + std::to_string(chainPosition), peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain " + std::to_string(chainPosition), peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality " + std::to_string(chainPosition), peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq " + std::to_string(chainPosition), lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq " + std::to_string(chainPosition), highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope " + std::to_string(chainPosition), lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope " + std::to_string(chainPosition), highCutSlopeSlider)
{
    // title setup
    title.setText("Filter", juce::dontSendNotification);
    title.setFont(24);

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

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
}

void FilterModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);

    g.setColour(juce::Colours::white);
    g.setFont(10);
    g.drawFittedText("LowCut", lowCutSlopeSlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("Peak", peakQualitySlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("HighCut", highCutSlopeSlider.getBounds(), juce::Justification::centredBottom, 1);
}

void FilterModuleGUI::resized()
{
    auto filtersArea = getContentRenderArea();

    auto titleAndBypassArea = filtersArea.removeFromTop(30);

    auto responseCurveArea = filtersArea.removeFromTop(filtersArea.getHeight() * (1.f / 2.f));
    responseCurveArea.reduce(10, 10);
    auto lowCutArea = filtersArea.removeFromLeft(filtersArea.getWidth() * (1.f / 3.f));
    auto highCutArea = filtersArea.removeFromRight(filtersArea.getWidth() * (1.f / 2.f));

    title.setBounds(titleAndBypassArea);
    title.setJustificationType(juce::Justification::centred);

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
    };
}
