/*
  ==============================================================================

    FFTAnalyzerComponent.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2021 Charles Schiermeyer
    Content: the original FFT Analyzer Component
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

#include "FFTAnalyzerComponent.h"

#include "../Shared/GUIStuff.h"

// component for the response curve in order to paint the curve only in his area
FFTAnalyzerComponent::FFTAnalyzerComponent(PluginState& p, unsigned int parameterNumber,
                                           Options analyzerOptions)
    : pluginState(p),
      options(analyzerOptions),
      leftPathProducer(*pluginState.leftAnalyzerFIFOs[0]),
      rightPathProducer(*pluginState.rightAnalyzerFIFOs[0])
{
    auto chainPosition = pluginState.getChainPositionFromDSPmodule(options.moduleType, parameterNumber);
    auto index = pluginState.getSampleFifoIndexOfCorrespondingModule(chainPosition);
    leftPathProducer.setSingleChannelSampleFifo(pluginState.leftAnalyzerFIFOs[index]);
    rightPathProducer.setSingleChannelSampleFifo(pluginState.rightAnalyzerFIFOs[index]);

    startTimerHz(59);
}

//==============================================================================
// view transform

float FFTAnalyzerComponent::frequencyToX(float frequency)
{
    const auto area = getAnalysysArea();
    const auto base = (float) area.getWidth() * juce::mapFromLog10(frequency, 20.0f, 20000.0f);

    return (float) area.getX() + base * horizontalZoom - panPixels;
}

float FFTAnalyzerComponent::levelToY(float levelDb)
{
    const auto area = getAnalysysArea();
    const auto base = juce::jmap(levelDb, minimumDb, 0.0f,
        (float) area.getHeight(), 0.0f);

    // vertical zoom is anchored at 0 dB (the top of the graph), then panned
    return (float) area.getY() + base * verticalZoom - panPixelsY;
}

float FFTAnalyzerComponent::getMaximumPan()
{
    return juce::jmax(0.0f, (float) getAnalysysArea().getWidth() * (horizontalZoom - 1.0f));
}

float FFTAnalyzerComponent::getMaximumPanY()
{
    return juce::jmax(0.0f, (float) getAnalysysArea().getHeight() * (verticalZoom - 1.0f));
}

bool FFTAnalyzerComponent::isPannable()
{
    return getMaximumPan() > 0.0f || getMaximumPanY() > 0.0f;
}

void FFTAnalyzerComponent::clampPan()
{
    panPixels = juce::jlimit(0.0f, getMaximumPan(), panPixels);
    panPixelsY = juce::jlimit(0.0f, getMaximumPanY(), panPixelsY);
}

void FFTAnalyzerComponent::setFrozen(bool shouldFreeze)
{
    frozen = shouldFreeze;
}

void FFTAnalyzerComponent::setViewZoom(float newHorizontalZoom, float newVerticalZoom)
{
    const auto clampedHorizontal = juce::jlimit(1.0f, 24.0f, newHorizontalZoom);
    const auto clampedVertical = juce::jlimit(1.0f, 4.0f, newVerticalZoom);

    if (juce::approximatelyEqual(clampedHorizontal, horizontalZoom)
        && juce::approximatelyEqual(clampedVertical, verticalZoom))
        return;

    // keep whatever is at the centre of the view centred while zooming
    const auto area = getAnalysysArea();
    const auto centreProportion = (panPixels + (float) area.getWidth() * 0.5f)
        / juce::jmax(1.0f, (float) area.getWidth() * horizontalZoom);

    horizontalZoom = clampedHorizontal;
    verticalZoom = clampedVertical;

    panPixels = centreProportion * (float) area.getWidth() * horizontalZoom
        - (float) area.getWidth() * 0.5f;
    clampPan();

    repaint();
}

void FFTAnalyzerComponent::resetView()
{
    panPixels = 0.0f;
    panPixelsY = 0.0f;
    repaint();
}

// Applies a zoom that came from a mouse or trackpad gesture and tells the owner, so the module's
// H/V zoom knobs stay in sync with what the user did on the graph.
void FFTAnalyzerComponent::applyGestureZoom(float newHorizontalZoom, float newVerticalZoom)
{
    horizontalZoom = juce::jlimit(1.0f, 24.0f, newHorizontalZoom);
    verticalZoom = juce::jlimit(1.0f, 4.0f, newVerticalZoom);
    clampPan();
    repaint();

    if (onViewZoomChangedByGesture != nullptr)
        onViewZoomChangedByGesture(horizontalZoom, verticalZoom);
}

void FFTAnalyzerComponent::panBy(float deltaX, float deltaY)
{
    if (! isPannable())
        return;

    panPixels += deltaX;
    panPixelsY += deltaY;
    clampPan();
    repaint();
}

void FFTAnalyzerComponent::updateViewCursor(bool dragging)
{
    const auto pannable = isPannable();

    setMouseCursor(pannable ? (dragging ? juce::MouseCursor::DraggingHandCursor
                                        : juce::MouseCursor::PointingHandCursor)
                            : juce::MouseCursor::NormalCursor);
    // the cursor only changes on the next mouse move unless it is refreshed explicitly
    updateMouseCursor();
}

void FFTAnalyzerComponent::mouseEnter(const juce::MouseEvent&)
{
    updateViewCursor(false);
}

void FFTAnalyzerComponent::mouseMove(const juce::MouseEvent&)
{
    updateViewCursor(false);
}

void FFTAnalyzerComponent::mouseExit(const juce::MouseEvent&)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void FFTAnalyzerComponent::mouseDown(const juce::MouseEvent&)
{
    panPixelsAtDragStart = panPixels;
    panPixelsYAtDragStart = panPixelsY;
    updateViewCursor(true);
}

void FFTAnalyzerComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (! isPannable())
        return;

    // drag travels along whichever axes are zoomed in
    panPixels = panPixelsAtDragStart - (float) event.getDistanceFromDragStartX();
    panPixelsY = panPixelsYAtDragStart - (float) event.getDistanceFromDragStartY();
    clampPan();
    repaint();
}

void FFTAnalyzerComponent::mouseUp(const juce::MouseEvent&)
{
    updateViewCursor(false);
}

void FFTAnalyzerComponent::mouseDoubleClick(const juce::MouseEvent&)
{
    // back to the start of the frequency range
    resetView();
}

// Zooms the frequency axis around the pointer, keeping whatever is under it in place.
void FFTAnalyzerComponent::zoomHorizontallyAround(float newHorizontalZoom, float anchorX)
{
    const auto area = getAnalysysArea();
    const auto pointerProportion = (panPixels + anchorX - (float) area.getX())
        / juce::jmax(1.0f, (float) area.getWidth() * horizontalZoom);

    const auto clamped = juce::jlimit(1.0f, 24.0f, newHorizontalZoom);

    panPixels = pointerProportion * (float) area.getWidth() * clamped
        - (anchorX - (float) area.getX());

    applyGestureZoom(clamped, verticalZoom);
}

// Zooms the level axis around the pointer, keeping whatever is under it in place.
void FFTAnalyzerComponent::zoomVerticallyAround(float newVerticalZoom, float anchorY)
{
    const auto area = getAnalysysArea();
    const auto pointerProportion = (panPixelsY + anchorY - (float) area.getY())
        / juce::jmax(1.0f, (float) area.getHeight() * verticalZoom);

    const auto clamped = juce::jlimit(1.0f, 4.0f, newVerticalZoom);

    panPixelsY = pointerProportion * (float) area.getHeight() * clamped
        - (anchorY - (float) area.getY());

    applyGestureZoom(horizontalZoom, clamped);
}

void FFTAnalyzerComponent::mouseWheelMove(const juce::MouseEvent& event,
                                          const juce::MouseWheelDetails& wheel)
{
    const auto direction = wheel.isReversed ? -1.0f : 1.0f;

    // a trackpad's sideways two finger scroll travels along the frequency axis
    if (! juce::approximatelyEqual(wheel.deltaX, 0.0f))
    {
        panBy(-wheel.deltaX * direction * (float) getAnalysysArea().getWidth() * 0.5f, 0.0f);
        return;
    }

    if (juce::approximatelyEqual(wheel.deltaY, 0.0f))
        return;

    // a mouse has no pinch gesture, so hold a modifier to zoom the frequency axis with the wheel
    if (event.mods.isCommandDown() || event.mods.isShiftDown() || event.mods.isAltDown())
    {
        zoomHorizontallyAround(horizontalZoom * (1.0f + wheel.deltaY * direction * 0.6f),
                               (float) event.position.x);
        return;
    }

    // plain two finger up/down scroll zooms the level axis, keeping the pointer anchored
    zoomVerticallyAround(verticalZoom * (1.0f + wheel.deltaY * direction * 0.5f),
                         (float) event.position.y);
}

void FFTAnalyzerComponent::mouseMagnify(const juce::MouseEvent& event, float scaleFactor)
{
    // trackpad pinch
    zoomHorizontallyAround(horizontalZoom * scaleFactor, (float) event.position.x);
}

//==============================================================================

void FFTAnalyzerComponent::timerCallback() {

    if (enableFFTanalysis && !frozen) {
        auto fftBounds = getAnalysysArea().toFloat();
        auto sampleRate = pluginState.audioProcessor.getSampleRate();
        leftPathProducer.process(fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);

        if (stereoView) {
            const auto& leftData = leftPathProducer.getLatestFFTData();
            const auto& rightData = rightPathProducer.getLatestFFTData();

            if (!leftData.empty() && leftData.size() == rightData.size()) {
                std::vector<float> stereoData(leftData.size());
                for (size_t index = 0; index < stereoData.size(); ++index) {
                    const auto magnitude = 0.5f * (juce::Decibels::decibelsToGain(leftData[index])
                        + juce::Decibels::decibelsToGain(rightData[index]));
                    stereoData[index] = juce::Decibels::gainToDecibels(magnitude, -48.0f);
                }

                const auto fftSize = leftPathProducer.getFFTSize();
                const auto binWidth = sampleRate / fftSize;
                stereoPathGenerator.generatePath(stereoData, fftBounds, fftSize,
                                                  static_cast<float>(binWidth), minimumDb);

                while (stereoPathGenerator.getNumPathsAvailable() > 0)
                    stereoPathGenerator.getPath(stereoFFTPath);
            }
        }

        repaint();
    }
}

// Renders one spectrum path: an optional gradient body, a soft glow and a smooth stroke.
void FFTAnalyzerComponent::drawSpectrum(juce::Graphics& g, const juce::Path& spectrumPath,
                                        juce::Rectangle<int> area, juce::Colour colour)
{
    using namespace juce;

    if (spectrumPath.isEmpty())
        return;

    auto path = spectrumPath;
    // the path is generated in graph-local coordinates, so the view transform (zoom + pan,
    // anchored at the graph's top-left) is applied before moving it into place
    path.applyTransform(AffineTransform::scale(horizontalZoom, verticalZoom)
        .translated((float) area.getX() - panPixels, (float) area.getY() - panPixelsY));

    if (options.fillUnderCurve)
    {
        auto filled = path;
        filled.lineTo((float) area.getRight(), (float) area.getBottom());
        filled.lineTo((float) area.getX(), (float) area.getBottom());
        filled.closeSubPath();

        g.setGradientFill(ColourGradient::vertical(colour.withAlpha(0.30f), (float) area.getY(),
                                                   colour.withAlpha(0.02f), (float) area.getBottom()));
        g.fillPath(filled);
    }

    // a wide, faint pass under the main stroke softens the aliasing of the thin line
    g.setColour(colour.withAlpha(0.22f));
    g.strokePath(path, PathStrokeType(3.2f, PathStrokeType::curved, PathStrokeType::rounded));

    g.setColour(colour);
    g.strokePath(path, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded));
}

void FFTAnalyzerComponent::paint(juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll(Colours::black);

    // The grid is drawn straight into the component instead of being cached in an image:
    // rebuilding that image on every zoom/pan event made the view stutter, and blitting a
    // scaled image made the labels blurry on a HiDPI screen.
    drawGrid(g);

    auto responseArea = getAnalysysArea();

    g.reduceClipRegion(responseArea);

    // FFT ANALYZER
    if (enableFFTanalysis) {
        if (stereoView) {
            drawSpectrum(g, stereoFFTPath, responseArea, options.stereoColour);
            return;
        }

        drawSpectrum(g, leftPathProducer.getPath(), responseArea, options.leftColour);
        drawSpectrum(g, rightPathProducer.getPath(), responseArea, options.rightColour);
    }
}

// +/-24 dB filter gain scale on the right, analyzer level on the left: used by the equalizer,
// where the same area also hosts the filter response curve.
void FFTAnalyzerComponent::drawFilterGainScale(juce::Graphics& g, juce::Rectangle<int> renderArea,
                                               int fontHeight)
{
    using namespace juce;

    const auto left = renderArea.getX();
    const auto right = renderArea.getRight();
    const auto top = renderArea.getY();
    const auto bottom = renderArea.getBottom();

    const Array<float> gains { -24, -12, 0, 12, 24 };

    for (auto gDb : gains) {
        const auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        const auto isZero = approximatelyEqual(gDb, 0.0f);

        g.setColour(isZero ? BiztortionTheme::controlAccent.withAlpha(0.55f)
                           : BiztortionTheme::border.withAlpha(0.4f));
        g.drawHorizontalLine(roundToInt(y), float(left), float(right));

        String str;
        if (gDb > 0)
            str << "+";
        str << gDb;

        auto textWidth = GlyphArrangement::getStringWidthInt(g.getCurrentFont(), str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), roundToInt(y));

        g.setColour(isZero ? BiztortionTheme::controlAccent : BiztortionTheme::textMuted);
        g.drawFittedText(str, r, Justification::centred, 1);

        // the analyzer itself spans -48..0 dB over the same height
        str.clear();
        str << (gDb - 24.f);
        r.setX(1);
        textWidth = GlyphArrangement::getStringWidthInt(g.getCurrentFont(), str);
        r.setSize(textWidth, fontHeight);
        g.setColour(BiztortionTheme::textMuted);
        g.drawFittedText(str, r, Justification::centred, 1);
    }
}

// plain dBFS scale for the standalone Spectrum Analyzer module
void FFTAnalyzerComponent::drawLevelScale(juce::Graphics& g, juce::Rectangle<int> renderArea,
                                          int fontHeight)
{
    using namespace juce;

    const auto left = renderArea.getX();
    const auto right = renderArea.getRight();
    const auto top = renderArea.getY();
    const auto bottom = renderArea.getBottom();

    for (auto levelDb : { 0.0f, -12.0f, -24.0f, -36.0f, minimumDb }) {
        const auto y = levelToY(levelDb);
        const auto isTop = approximatelyEqual(levelDb, 0.0f);

        // vertical zoom pushes the lower levels out of view
        if (y > (float) bottom + 1.0f)
            continue;

        g.setColour(BiztortionTheme::border.withAlpha(isTop ? 0.7f : 0.4f));
        g.drawHorizontalLine(roundToInt(y), float(left), float(right));

        String str;
        str << roundToInt(levelDb);

        const auto textWidth = GlyphArrangement::getStringWidthInt(g.getCurrentFont(), str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth - 1);
        // keep the extreme labels inside the component instead of hanging off the edge
        r.setCentre(r.getCentreX(), jlimit(top + fontHeight / 2, bottom - fontHeight / 2, roundToInt(y)));

        g.setColour(BiztortionTheme::textMuted);
        g.drawFittedText(str, r, Justification::centred, 1);
    }
}

void FFTAnalyzerComponent::resized()
{
    // the visible frequency span depends on the width, so a resize can leave the pan dangling
    clampPan();
}

void FFTAnalyzerComponent::drawGrid(juce::Graphics& g)
{
    using namespace juce;

    if (getWidth() <= 0 || getHeight() <= 0)
        return;

    g.fillAll(BiztortionTheme::canvas);

    auto renderArea = getAnalysysArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();

    const int fontHeight = 10;
    g.setFont(Font("Segoe UI", (float) fontHeight, Font::plain));

    // --- FREQUENCY GRID ---

    Array<float> freqs
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };
    // caching x infos related to the container dimensions in an array (through the view
    // transform, so the grid follows the zoom and pan of the spectrum)
    Array<float> xs;
    for (auto f : freqs) {
        xs.add(frequencyToX(f));
    }

    for (int i = 0; i < xs.size(); ++i) {
        if (xs[i] < (float) left - 1.0f || xs[i] > (float) right + 1.0f)
            continue;

        // decades get a slightly stronger line so the scale stays readable while staying subtle
        const auto isDecade = approximatelyEqual(freqs[i], 100.0f)
            || approximatelyEqual(freqs[i], 1000.0f)
            || approximatelyEqual(freqs[i], 10000.0f);
        g.setColour(BiztortionTheme::border.withAlpha(isDecade ? 0.7f : 0.35f));
        g.drawVerticalLine(roundToInt(xs[i]), float(top), float(bottom));
    }

    // --- LEVEL / GAIN SCALE ---

    if (options.showFilterGainScale)
        drawFilterGainScale(g, renderArea, fontHeight);
    else
        drawLevelScale(g, renderArea, fontHeight);

    // --- FREQ LABELS ---

    g.setColour(BiztortionTheme::textMuted);

    for (int i = 0; i < freqs.size(); ++i) {
        auto f = freqs[i];
        auto x = xs[i];

        // skip the labels the current zoom / pan has pushed outside the graph
        if (x < (float) left - 1.0f || x > (float) right + 1.0f)
            continue;

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

        auto textWidth = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(),str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }

    // --- ANALYZER VIEW LABELS ---
    String str(stereoView ? "Stereo" : "L");
    g.setFont(juce::Font("Segoe UI", (float) fontHeight, juce::Font::bold));
    auto textWidth = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(),str);
    Rectangle<int> r;
    r.setSize(textWidth, fontHeight);
    r.setCentre(left + textWidth / 2 + 3, top + 8);
    g.setColour(stereoView ? options.stereoColour : options.leftColour);
    g.drawFittedText(str, r, juce::Justification::centred, 1);

    if (stereoView)
        return;

    str.clear();
    str << "R";
    textWidth = juce::GlyphArrangement::getStringWidthInt(g.getCurrentFont(),str);
    r.setSize(textWidth, fontHeight);
    r.setCentre(left + 17, top + 8);
    g.setColour(options.rightColour);
    g.drawFittedText(str, r, juce::Justification::centred, 1);
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

void FFTAnalyzerComponent::setStereoView(bool shouldShowStereo)
{
    stereoView = shouldShowStereo;
    resized();
    repaint();
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
