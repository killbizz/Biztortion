/*
  ==============================================================================

    FilterModule.h
    Created: 17 Aug 2021 4:12:53pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "SingleModule.h"
#include "../Component/ResponseCurveComponent.h"
#include "../Component/FFTAnalyzerComponent.h"
#include "../Component/GUIStuff.h"

//==============================================================================

/* FilterModule DSP */

//==============================================================================

enum FilterSlope {
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct FilterChainSettings {
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    int lowCutSlope{ FilterSlope::Slope_12 }, highCutSlope{ FilterSlope::Slope_12 };
};

using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

enum ChainPositions {
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacements);

template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& monoChain, const CoefficientType& cutCoefficients, const FilterSlope& slope) {
    monoChain.setBypassed<0>(true);
    monoChain.setBypassed<1>(true);
    monoChain.setBypassed<2>(true);
    monoChain.setBypassed<3>(true);
    switch (slope) {
    case Slope_12:
        *monoChain.get<0>().coefficients = *cutCoefficients[0];
        monoChain.setBypassed<0>(false);
        break;
    case Slope_24:
        *monoChain.get<0>().coefficients = *cutCoefficients[0];
        monoChain.setBypassed<0>(false);
        *monoChain.get<1>().coefficients = *cutCoefficients[1];
        monoChain.setBypassed<1>(false);
        break;
    case Slope_36:
        *monoChain.get<0>().coefficients = *cutCoefficients[0];
        monoChain.setBypassed<0>(false);
        *monoChain.get<1>().coefficients = *cutCoefficients[1];
        monoChain.setBypassed<1>(false);
        *monoChain.get<2>().coefficients = *cutCoefficients[2];
        monoChain.setBypassed<2>(false);
        break;
    case Slope_48:
        *monoChain.get<0>().coefficients = *cutCoefficients[0];
        monoChain.setBypassed<0>(false);
        *monoChain.get<1>().coefficients = *cutCoefficients[1];
        monoChain.setBypassed<1>(false);
        *monoChain.get<2>().coefficients = *cutCoefficients[2];
        monoChain.setBypassed<2>(false);
        *monoChain.get<3>().coefficients = *cutCoefficients[3];
        monoChain.setBypassed<3>(false);
        break;
    }
}

class FilterModuleDSP {
public:
    FilterModuleDSP(juce::AudioProcessorValueTreeState&);
    // inline for avoiding linking problems with functions which have declaration + impementation
    // in the file.h (placed here for convenience)
    static inline auto makeLowCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
        // i need explanation ...
        return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
            chainSettings.lowCutFreq, sampleRate, 2 * (chainSettings.lowCutSlope + 1));
    }
    static inline auto makeHighCutFilter(const FilterChainSettings& chainSettings, double sampleRate) {
        // i need explanation ...
        return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
            chainSettings.highCutFreq, sampleRate, 2 * (chainSettings.highCutSlope + 1));
    }
    static Coefficients makePeakFilter(const FilterChainSettings& chainSettings, double sampleRate);

    static FilterChainSettings getFilterChainSettings(juce::AudioProcessorValueTreeState& apvts);

    static void addFilterParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);

    void updateFilters(double);
    void updatePeakFilter(const FilterChainSettings& chainSettings, double sampleRate);
    void updateLowCutFilter(const FilterChainSettings& chainSettings, double sampleRate);
    void updateHighCutFilter(const FilterChainSettings& chainSettings, double sampleRate);

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double);

private:
    juce::AudioProcessorValueTreeState& apvts;
    MonoChain leftChain, rightChain;
};

//==============================================================================

/* FilterModule GUI */

//==============================================================================

class FilterModuleGUI : public SingleModule {
public:
    FilterModuleGUI(BiztortionAudioProcessor&);

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::vector<juce::Component*> getComps() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BiztortionAudioProcessor& audioProcessor;

    // filterModule
    RotarySliderWithLabels peakFreqSlider,
        peakGainSlider,
        peakQualitySlider,
        lowCutFreqSlider,
        highCutFreqSlider,
        lowCutSlopeSlider,
        highCutSlopeSlider;
    Attachment peakFreqSliderAttachment,
        peakGainSliderAttachment,
        peakQualitySliderAttachment,
        lowCutFreqSliderAttachment,
        highCutFreqSliderAttachment,
        lowCutSlopeSliderAttachment,
        highCutSlopeSliderAttachment;
    ResponseCurveComponent responseCurveComponent;
    FFTAnalyzerComponent filterFftAnalyzerComponent;

};