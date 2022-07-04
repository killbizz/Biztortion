/*
  ==============================================================================

    FilterModule.h

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

#pragma once

#include <JuceHeader.h>

#include "../Component/ResponseCurveComponent.h"
#include "../Component/FFTAnalyzerComponent.h"
#include "../Shared/GUIStuff.h"
#include "../Module/DSPModule.h"
#include "../Module/GUIModule.h"
#include "../Shared/PluginState.h"

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
    bool bypassed{ false }, analyzerBypassed{ false };
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
    monoChain. template setBypassed<0>(true);
    monoChain. template setBypassed<1>(true);
    monoChain. template setBypassed<2>(true);
    monoChain. template setBypassed<3>(true);
    switch (slope) {
    case Slope_12:
        *monoChain. template get<0>().coefficients = *cutCoefficients[0];
        monoChain. template setBypassed<0>(false);
        break;
    case Slope_24:
        *monoChain. template get<0>().coefficients = *cutCoefficients[0];
        monoChain. template setBypassed<0>(false);
        *monoChain. template get<1>().coefficients = *cutCoefficients[1];
        monoChain. template setBypassed<1>(false);
        break;
    case Slope_36:
        *monoChain. template get<0>().coefficients = *cutCoefficients[0];
        monoChain. template setBypassed<0>(false);
        *monoChain. template get<1>().coefficients = *cutCoefficients[1];
        monoChain. template setBypassed<1>(false);
        *monoChain. template get<2>().coefficients = *cutCoefficients[2];
        monoChain. template setBypassed<2>(false);
        break;
    case Slope_48:
        *monoChain. template get<0>().coefficients = *cutCoefficients[0];
        monoChain. template setBypassed<0>(false);
        *monoChain. template get<1>().coefficients = *cutCoefficients[1];
        monoChain. template setBypassed<1>(false);
        *monoChain. template get<2>().coefficients = *cutCoefficients[2];
        monoChain. template setBypassed<2>(false);
        *monoChain. template get<3>().coefficients = *cutCoefficients[3];
        monoChain. template setBypassed<3>(false);
        break;
    }
}

class FilterModuleDSP : public DSPModule {
public:
    FilterModuleDSP(juce::AudioProcessorValueTreeState& _apvts);
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

    static FilterChainSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
    MonoChain* getOneChain();

    void updateDSPState(double sampleRate) override;
    void updatePeakFilter(const FilterChainSettings& chainSettings, double sampleRate);
    void updateLowCutFilter(const FilterChainSettings& chainSettings, double sampleRate);
    void updateHighCutFilter(const FilterChainSettings& chainSettings, double sampleRate);

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&, double) override;

private:
    MonoChain leftChain, rightChain;
    bool bypassed = false;
};

//==============================================================================

/* FilterModule GUI */

//==============================================================================

class FilterModuleGUI : public GUIModule {
public:
    FilterModuleGUI(PluginState& p, unsigned int parameterNumber);
    ~FilterModuleGUI();

    std::vector<juce::Component*> getAllComps() override;
    virtual std::vector<juce::Component*> getParamComps() override;
    virtual void updateParameters(const juce::Array<juce::var>& values) override;
    virtual void resetParameters(unsigned int parameterNumber) override;
    virtual juce::Array<juce::var> getParamValues() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;
    using ButtonAttachment = APVTS::ButtonAttachment;

    PluginState& pluginState;

    juce::Label title;

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

    PowerButton bypassButton;
    AnalyzerButton analyzerButton;

    ButtonAttachment bypassButtonAttachment,
        analyzerButtonAttachment;

    ButtonsLookAndFeel lnf;
    
    ResponseCurveComponent responseCurveComponent;
    FFTAnalyzerComponent filterFftAnalyzerComponent;

};