/*
  ==============================================================================

    ResponseCurveComponent.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2021 Charles Schiermeyer
    Content: the original Response Curve Component
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

#include "ResponseCurveComponent.h"

#include "../Module/EqualizerModule/EqualizerModule.h"


// component for the response curve in order to paint the curve only in his area
ResponseCurveComponent::ResponseCurveComponent(PluginState& p, unsigned int parameterNumber)
    : pluginState(p), parameterNumber(parameterNumber)
{

    const auto& params = pluginState.audioProcessor.getParameters();
    for (auto param : params) {
        if (param->getLabel() == juce::String("Filter " + std::to_string(parameterNumber))) {
            param->addListener(this);
        }
    }
    updateChain();

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent() {
    const auto& params = pluginState.audioProcessor.getParameters();
    for (auto param : params) {
        if (param->getLabel() == juce::String("Filter " + std::to_string(parameterNumber))) {
            param->removeListener(this);
        }
    }
}


void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parameterChanged.set(true);
}

void ResponseCurveComponent::timerCallback() {

    if (parameterChanged.compareAndSetBool(false, true)) {
        updateChain();
        updateResponseCurve();
    }
    repaint();
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black.withAlpha(0.f));

    // drawing response curve
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));

    // --- band handles ---
    const auto accent = moduleType_colors.at(ModuleType::Equalizer);

    for (const auto& handle : getBandHandles())
    {
        const auto isActive = !handle.bypassed && !moduleBypassed;
        const auto isHovered = handle.band == hoveredBand || handle.band == draggedBand;
        const auto radius = handleRadius + (isHovered ? 1.5f : 0.0f);
        const auto area = Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(handle.position);

        // a soft halo makes the handle readable on top of the spectrum
        g.setColour((isActive ? accent : BiztortionTheme::textMuted).withAlpha(isHovered ? 0.35f : 0.2f));
        g.fillEllipse(area.expanded(3.0f));

        g.setColour(isActive ? accent : BiztortionTheme::surfaceRaised);
        g.fillEllipse(area);

        g.setColour(isActive ? Colours::white : BiztortionTheme::textMuted);
        g.drawEllipse(area, isHovered ? 2.0f : 1.4f);

        // a bypassed band keeps its handle, struck through
        if (!isActive)
        {
            const auto slash = area.reduced(radius * 0.32f);
            g.drawLine(slash.getX(), slash.getBottom(), slash.getRight(), slash.getY(), 1.4f);
        }
    }
}

//==============================================================================
// band handles

float ResponseCurveComponent::frequencyToX(float frequency)
{
    const auto area = getAnalysysArea();

    return (float) area.getX()
        + (float) area.getWidth() * juce::mapFromLog10(frequency, 20.0f, 20000.0f);
}

float ResponseCurveComponent::xToFrequency(float x)
{
    const auto area = getAnalysysArea();
    const auto proportion = juce::jlimit(0.0f, 1.0f,
        ((float) x - (float) area.getX()) / (float) juce::jmax(1, area.getWidth()));

    return juce::mapToLog10(proportion, 20.0f, 20000.0f);
}

float ResponseCurveComponent::gainToY(float gainDb)
{
    const auto area = getAnalysysArea();

    return juce::jmap(juce::jlimit(-maximumGainDb, maximumGainDb, gainDb),
        -maximumGainDb, maximumGainDb, (float) area.getBottom(), (float) area.getY());
}

float ResponseCurveComponent::yToGainDb(float y)
{
    const auto area = getAnalysysArea();

    return juce::jlimit(-maximumGainDb, maximumGainDb,
        juce::jmap((float) y, (float) area.getBottom(), (float) area.getY(),
                   -maximumGainDb, maximumGainDb));
}

juce::String ResponseCurveComponent::getBandParameterID(Band band, const juce::String& parameter) const
{
    const auto number = juce::String(static_cast<int>(parameterNumber));

    switch (band)
    {
        case Band::lowCut:  return "LowCut " + parameter + " " + number;
        case Band::peak1:   return "Peak1 " + parameter + " " + number;
        case Band::peak2:   return "Peak2 " + parameter + " " + number;
        case Band::highCut: return "HighCut " + parameter + " " + number;
        default:            return {};
    }
}

juce::RangedAudioParameter* ResponseCurveComponent::getBandParameter(Band band,
                                                                    const juce::String& parameter) const
{
    const auto id = getBandParameterID(band, parameter);

    return id.isEmpty() ? nullptr : pluginState.apvts.getParameter(id);
}

std::vector<ResponseCurveComponent::BandHandle> ResponseCurveComponent::getBandHandles()
{
    std::vector<BandHandle> handles;

    const auto settings = EqualizerModuleDSP::getSettings(pluginState.apvts, parameterNumber);
    // cut filters have no gain, so their handles ride the 0 dB line
    const auto zeroGainY = gainToY(0.0f);

    handles.push_back({ Band::lowCut, { frequencyToX(settings.lowCutFreq), zeroGainY },
                        settings.lowCutBypassed, false });
    handles.push_back({ Band::peak1,
                        { frequencyToX(settings.peak1Freq), gainToY(settings.peak1GainInDecibels) },
                        settings.peak1Bypassed, true });
    handles.push_back({ Band::peak2,
                        { frequencyToX(settings.peak2Freq), gainToY(settings.peak2GainInDecibels) },
                        settings.peak2Bypassed, true });
    handles.push_back({ Band::highCut, { frequencyToX(settings.highCutFreq), zeroGainY },
                        settings.highCutBypassed, false });

    return handles;
}

ResponseCurveComponent::Band ResponseCurveComponent::findBandAt(juce::Point<float> position)
{
    // single gate for dragging, double-clicking and hovering: while the module is bypassed
    // there is nothing to edit, so no handle is ever hit
    if (moduleBypassed)
        return Band::none;

    auto closest = Band::none;
    auto closestDistance = handleHitRadius;

    for (const auto& handle : getBandHandles())
    {
        const auto distance = handle.position.getDistanceFrom(position);

        if (distance <= closestDistance)
        {
            closestDistance = distance;
            closest = handle.band;
        }
    }

    return closest;
}

void ResponseCurveComponent::setBandParameterValue(Band band, const juce::String& parameter, float value)
{
    if (auto* param = getBandParameter(band, parameter))
        param->setValueNotifyingHost(param->convertTo0to1(value));
}

void ResponseCurveComponent::mouseDown(const juce::MouseEvent& event)
{
    draggedBand = findBandAt(event.position);

    if (draggedBand == Band::none)
        return;

    if (auto* freq = getBandParameter(draggedBand, "Freq"))
        freq->beginChangeGesture();

    if (draggedBand == Band::peak1 || draggedBand == Band::peak2)
        if (auto* gain = getBandParameter(draggedBand, "Gain"))
            gain->beginChangeGesture();

    repaint();
}

void ResponseCurveComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (draggedBand == Band::none)
        return;

    setBandParameterValue(draggedBand, "Freq", xToFrequency(event.position.x));

    if (draggedBand == Band::peak1 || draggedBand == Band::peak2)
        setBandParameterValue(draggedBand, "Gain", yToGainDb(event.position.y));
}

void ResponseCurveComponent::mouseUp(const juce::MouseEvent&)
{
    if (draggedBand == Band::none)
        return;

    if (auto* freq = getBandParameter(draggedBand, "Freq"))
        freq->endChangeGesture();

    if (draggedBand == Band::peak1 || draggedBand == Band::peak2)
        if (auto* gain = getBandParameter(draggedBand, "Gain"))
            gain->endChangeGesture();

    draggedBand = Band::none;
    repaint();
}

void ResponseCurveComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    const auto band = findBandAt(event.position);

    if (band == Band::none)
        return;

    if (auto* bypassed = getBandParameter(band, "Bypassed"))
    {
        const auto isBypassed = bypassed->getValue() > 0.5f;
        bypassed->setValueNotifyingHost(isBypassed ? 0.0f : 1.0f);
    }

    repaint();
}

// Scrolling on a handle tightens or widens that band: Q for the peaks, slope for the cuts.
void ResponseCurveComponent::mouseWheelMove(const juce::MouseEvent& event,
                                            const juce::MouseWheelDetails& wheel)
{
    const auto band = findBandAt(event.position);

    if (band == Band::none || juce::approximatelyEqual(wheel.deltaY, 0.0f))
    {
        // not on a handle: let whatever is underneath deal with it
        Component::mouseWheelMove(event, wheel);
        return;
    }

    const auto direction = (wheel.deltaY > 0.0f ? 1.0f : -1.0f) * (wheel.isReversed ? -1.0f : 1.0f);
    const auto isPeak = band == Band::peak1 || band == Band::peak2;

    if (auto* param = getBandParameter(band, isPeak ? "Quality" : "Slope"))
    {
        const auto& range = param->getNormalisableRange();

        param->beginChangeGesture();

        if (isPeak)
        {
            // Step in normalised space. Stepping by a proportion of the value instead made the
            // step smaller than the parameter's snap interval at low Q, so snapToLegalValue
            // rounded straight back and the control got stuck around 0.2.
            param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, param->getValue() + direction * 0.03f));
        }
        else
        {
            // the cut slope is a choice parameter: move it one option at a time
            const auto current = range.convertFrom0to1(param->getValue());
            const auto step = range.interval > 0.0f ? range.interval : 1.0f;
            const auto target = range.snapToLegalValue(current + direction * step);

            param->setValueNotifyingHost(range.convertTo0to1(target));
        }

        param->endChangeGesture();
    }

    repaint();
}

void ResponseCurveComponent::mouseMove(const juce::MouseEvent& event)
{
    const auto band = findBandAt(event.position);

    if (band != hoveredBand)
    {
        hoveredBand = band;
        setMouseCursor(band == Band::none ? juce::MouseCursor::NormalCursor
                                          : juce::MouseCursor::PointingHandCursor);
        repaint();
    }
}

void ResponseCurveComponent::mouseExit(const juce::MouseEvent&)
{
    if (hoveredBand != Band::none)
    {
        hoveredBand = Band::none;
        repaint();
    }
}

void ResponseCurveComponent::resized()
{
    using namespace juce;

    responseCurve.preallocateSpace(getWidth() * 3);
    updateResponseCurve();
}

void ResponseCurveComponent::updateResponseCurve()
{
    auto responseArea = getAnalysysArea();
    auto responseWidth = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak1 = monoChain.get<ChainPositions::Peak1>();
    auto& peak2 = monoChain.get<ChainPositions::Peak2>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = pluginState.audioProcessor.getSampleRate();

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
        // getting magnitude for each frequency if filter isn't bypassed
        if (!monoChain.isBypassed<ChainPositions::Peak1>()) {
            mag *= peak1.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!monoChain.isBypassed<ChainPositions::Peak2>()) {
            mag *= peak2.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!monoChain.isBypassed<ChainPositions::LowCut>()) {
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
        }
        if (!monoChain.isBypassed<ChainPositions::HighCut>()) {
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
        }
        magnitudes[i] = Decibels::gainToDecibels(mag);
    }

    responseCurve.clear();

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    // lambda function
    auto map = [outputMin, outputMax](double input) {
        // remap of the sliders range to the responseArea y range
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    // creation of the line path
    if (magnitudes.size() > 0) {
        responseCurve.startNewSubPath(responseArea.getX(), jlimit(outputMax, outputMin, map(magnitudes.front())));
        for (size_t i = 1; i < magnitudes.size(); ++i) {
            // clipping response curve
            responseCurve.lineTo(responseArea.getX() + i, jlimit(outputMax, outputMin, map(magnitudes[i])));
        }
    }
}

void ResponseCurveComponent::updateChain()
{
    auto chainSettings = EqualizerModuleDSP::getSettings(pluginState.apvts, parameterNumber);

    auto sampleRate = pluginState.audioProcessor.getSampleRate();

    // peak filters
    auto peak1Coefficients = EqualizerModuleDSP::makePeakFilter(ChainPositions::Peak1, chainSettings, sampleRate);
    auto peak2Coefficients = EqualizerModuleDSP::makePeakFilter(ChainPositions::Peak2, chainSettings, sampleRate);
    updateCoefficients(monoChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    updateCoefficients(monoChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);

    auto lowCutCoefficients = EqualizerModuleDSP::makeLowCutFilter(chainSettings, sampleRate);
    auto highCutCoefficients = EqualizerModuleDSP::makeHighCutFilter(chainSettings, sampleRate);

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(),
        lowCutCoefficients,
        static_cast<FilterSlope>(chainSettings.lowCutSlope));

    updateCutFilter(monoChain.get<ChainPositions::HighCut>(),
        highCutCoefficients,
        static_cast<FilterSlope>(chainSettings.highCutSlope));

    monoChain.setBypassed<ChainPositions::Peak1>(chainSettings.peak1Bypassed || chainSettings.bypassed);
    monoChain.setBypassed<ChainPositions::Peak2>(chainSettings.peak2Bypassed || chainSettings.bypassed);
    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed || chainSettings.bypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed || chainSettings.bypassed);

    // a bypassed module has no filters to edit, so its handles go inert as well
    moduleBypassed = chainSettings.bypassed;
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
