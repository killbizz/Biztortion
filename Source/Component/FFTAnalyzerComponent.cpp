/*
  ==============================================================================

    FFTAnalyzerComponent.cpp
    Created: 24 Aug 2021 5:21:17pm
    Author:  gabri

  ==============================================================================
*/

#include "FFTAnalyzerComponent.h"
#include "../PluginProcessor.h"

// component for the response curve in order to paint the curve only in his area
FFTAnalyzerComponent::FFTAnalyzerComponent(BiztortionAudioProcessor& p, unsigned int chainPosition)
    : audioProcessor(p), 
      leftPathProducer(*audioProcessor.leftAnalyzerFIFOs[0]),
      rightPathProducer(*audioProcessor.rightAnalyzerFIFOs[0])
{
    auto index = audioProcessor.getFftAnalyzerFifoIndexOfCorrespondingFilter(chainPosition);
    leftPathProducer.setSingleChannelSampleFifo(audioProcessor.leftAnalyzerFIFOs[index]);
    rightPathProducer.setSingleChannelSampleFifo(audioProcessor.rightAnalyzerFIFOs[index]);

    startTimerHz(59);
}

void FFTAnalyzerComponent::timerCallback() {

    if (enableFFTanalysis) {
        auto fftBounds = getAnalysysArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();
        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);

        repaint();
    }
}

void FFTAnalyzerComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll(Colours::black);

    // BACKGROUND
    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysysArea();

    g.reduceClipRegion(responseArea);

    // FFT ANALYZER
    if (enableFFTanalysis) {
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform()
            .translated(responseArea.getX(), responseArea.getY())
        );
        g.setColour(Colours::skyblue);
        g.strokePath(leftChannelFFTPath, PathStrokeType(1));

        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(),
            responseArea.getY()));
        g.setColour(Colours::lightyellow);
        g.strokePath(rightChannelFFTPath, PathStrokeType(1));
    }
}

void FFTAnalyzerComponent::resized()
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

    // --- GRID LINES ---

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

    // --- FREQ LABELS ---

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

    // --- GAIN LABELS ---

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

        // --- ANALYZER LABELS ---

        str.clear();
        str << (gDb - 24.f);
        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
}

PathProducer& FFTAnalyzerComponent::getLeftPathProducer()
{
    return leftPathProducer;
}

PathProducer& FFTAnalyzerComponent::getRightPathProducer()
{
    return rightPathProducer;
}

void FFTAnalyzerComponent::toggleFFTanaysis(bool shouldEnableFFTanalysis)
{
    enableFFTanalysis = shouldEnableFFTanalysis;
}

juce::Rectangle<int> FFTAnalyzerComponent::getRenderArea()
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

juce::Rectangle<int> FFTAnalyzerComponent::getAnalysysArea()
{
    // renderArea = container ; analysysArea = content of responseCurve and FFT analyzer
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);

    return bounds;
}
