/*
  ==============================================================================

    ResponseCurveComponent.cpp
    Created: 17 Aug 2021 6:24:03pm
    Author:  gabri

  ==============================================================================
*/

#include "ResponseCurveComponent.h"
#include "../PluginProcessor.h"

//==============================================================================

/* ResponseCurveComponent */

//==============================================================================

// component for the response curve in order to paint the curve only in his area
ResponseCurveComponent::ResponseCurveComponent(BiztortionAudioProcessor& p, unsigned int chainPosition)
    : audioProcessor(p), chainPosition(chainPosition)
{
    setFilterMonoChain();

    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        if (param->getLabel() == juce::String("Filter " + std::to_string(chainPosition))) {
            param->addListener(this);
        }
    }
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent() {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        if (param->getLabel() == juce::String("Filter " + std::to_string(chainPosition))) {
            param->removeListener(this);
        }
    }
}


void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parameterChanged.set(true);
}

void ResponseCurveComponent::timerCallback() {

    if (parameterChanged.compareAndSetBool(false, true)) {
        // signal a repaint
        repaint();
    }
}

//void ResponseCurveComponent::monoChainUpdate()
//{
//    // update the monoChain
//    auto chainSettings = FilterModuleDSP::getSettings(audioProcessor.apvts, type);
//    auto peakCoefficients = FilterModuleDSP::makePeakFilter(chainSettings, audioProcessor.getSampleRate());
//    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
//
//    auto lowCutCoefficients = FilterModuleDSP::makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
//    auto highCutCoefficients = FilterModuleDSP::makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
//
//    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients,
//        static_cast<FilterSlope>(chainSettings.lowCutSlope));
//    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients,
//        static_cast<FilterSlope>(chainSettings.highCutSlope));
//}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black.withAlpha(0.f));

    auto responseArea = getAnalysysArea();
    auto responseWidth = responseArea.getWidth();

    auto& lowcut = filterMonoChain->get<ChainPositions::LowCut>();
    auto& peak = filterMonoChain->get<ChainPositions::Peak>();
    auto& highcut = filterMonoChain->get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

    // RESPONSE AREA AND RESPONSE CURVE

    std::vector<double> magnitudes;
    // one magnitude per pixel
    magnitudes.resize(responseWidth);
    // compute magnitude for each pixel at that frequency
    // magnitude is expressed in gain units => multiplicative (!= decibels)
    for (int i = 0; i < responseWidth; ++i) {
        // starting gain of 1
        double mag = 1.f;
        // normalising pixel number = double(i) / double(responseWidth)
        auto freq = mapToLog10(double(i) / double(responseWidth), 20.0, 20000.0);
        // getting magnitude for each frequency
        if (!filterMonoChain->isBypassed<ChainPositions::Peak>()) {
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!lowcut.isBypassed<0>()) {
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowcut.isBypassed<1>()) {
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowcut.isBypassed<2>()) {
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowcut.isBypassed<3>()) {
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!highcut.isBypassed<0>()) {
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highcut.isBypassed<1>()) {
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highcut.isBypassed<2>()) {
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highcut.isBypassed<3>()) {
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        magnitudes[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    // lambda function
    auto map = [outputMin, outputMax](double input) {
        // remap of the sliders range to the responseArea y range
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    // creation of the line path
    responseCurve.startNewSubPath(responseArea.getX(), map(magnitudes.front()));
    for (size_t i = 1; i < magnitudes.size(); ++i) {
        responseCurve.lineTo(responseArea.getX() + i, map(magnitudes[i]));
    }
    // drawing response area
     g.setColour(Colours::orange);
     g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
    // drawing response curve
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

}

void ResponseCurveComponent::resized()
{

}

void ResponseCurveComponent::setFilterMonoChain()
{
    for (auto it = audioProcessor.DSPmodules.cbegin(); it < audioProcessor.DSPmodules.cend(); ++it) {
        auto temp = dynamic_cast<FilterModuleDSP*>(&(**it));
        if (temp && temp->getChainPosition() == chainPosition) {
            filterMonoChain = temp->getOneChain();
        }
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    // returns a dimesion reduced rectangle as bounds in order to avoid margin collisions
    auto bounds = getLocalBounds();

    //bounds.reduce(10, //JUCE_LIVE_CONSTANT(5), 
    //    8 //JUCE_LIVE_CONSTANT(5)
    //    );

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysysArea()
{
    // renderArea = container ; analysysArea = content of responseCurve and FFT analyzer
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);

    return bounds;
}
