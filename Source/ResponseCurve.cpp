/*
  ==============================================================================

    ResponseCurve.cpp
    Created: 17 Aug 2021 6:24:03pm
    Author:  gabri

  ==============================================================================
*/

#include "ResponseCurve.h"

//==============================================================================

/* ResponseCurveComponent */

//==============================================================================

// component for the response curve in order to paint the curve only in his area
ResponseCurveComponent::ResponseCurveComponent(BiztortionAudioProcessor& p) : audioProcessor(p) {
    monoChainUpdate();
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        if (param->getLabel() == juce::String("Filter")) {
            param->addListener(this);
            std::cout << param->getName(100);
        }
    }
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent() {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        if (param->getLabel() == juce::String("Filter")) {
            param->removeListener(this);
        }
    }
}


void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parameterChanged.set(true);
}

void ResponseCurveComponent::timerCallback() {
    if (parameterChanged.compareAndSetBool(false, true)) {
        monoChainUpdate();
        // signal a repaint
        repaint();
    }
}

void ResponseCurveComponent::monoChainUpdate()
{
    // update the monoChain
    auto chainSettings = FilterModule::getFilterChainSettings(audioProcessor.apvts);
    auto peakCoefficients = FilterModule::makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficients = FilterModule::makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = FilterModule::makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients,
        static_cast<FilterSlope>(chainSettings.lowCutSlope));
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients,
        static_cast<FilterSlope>(chainSettings.highCutSlope));
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);

    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysysArea();
    auto responseWidth = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate();

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
        if (!monoChain.isBypassed<ChainPositions::Peak>()) {
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
    using namespace juce;

    // --- BACKGROUND GRID ---

    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

    Graphics g(background);

    auto renderArea = getAnalysysArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    // GRID LINES

    g.setColour(Colours::dimgrey);

    Array<float> freqs
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };
    // caching x infos related to the container dimensions in an array
    Array<float> xs;
    for (auto f : freqs) {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }
    for (auto x : xs) {
        g.drawVerticalLine(x, top, bottom);
    }

    Array<float> gains
    {
        -24, -12, 0, 12, 24
    };
    for (auto gDb : gains) {
        // range of sliders = -24, +24
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }

    // FREQ LABELS

    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);

    for (int i = 0; i < freqs.size(); ++i) {
        auto f = freqs[i];
        auto x = xs[i];

        bool addKHz = false;
        String str;
        if (f > 999.f) {
            addKHz = true;
            f /= 1000.f;
        }
        str << f;
        if (addKHz) {
            str << "k";
        }
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    // GAIN LABELS

    for (auto gDb : gains) {
        // range of sliders = -24, +24
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        String str;
        if (gDb > 0) {
            str << "+";
        }
        str << gDb;
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, 1);

        // ANALYZER LABELS

        str.clear();
        str << (gDb - 24.f);
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
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
    // renderArea = container ; analysysArea = content of responseCurve
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);

    return bounds;
}
