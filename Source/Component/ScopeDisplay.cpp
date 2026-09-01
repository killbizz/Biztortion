/*
  ==============================================================================

    ScopeDisplay.cpp

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

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

#include "ScopeDisplay.h"

#include "../Shared/GUIStuff.h"

//==============================================================================

ScopeSampleBuffer::ScopeSampleBuffer()
{
    history.setSize(2, historySize, false, true, false);
    history.clear();
}

void ScopeSampleBuffer::push(const float* left, const float* right, int numSamples)
{
    if (numSamples <= 0)
        return;

    auto position = writePosition.load(std::memory_order_relaxed);

    for (int offset = 0; offset < numSamples;)
    {
        const auto chunk = juce::jmin(numSamples - offset, historySize - position);

        history.copyFrom(0, position, left + offset, chunk);
        history.copyFrom(1, position, right + offset, chunk);

        position = (position + chunk) % historySize;
        offset += chunk;
    }

    writePosition.store(position, std::memory_order_release);
}

float ScopeSampleBuffer::getSample(int channel, int indexFromOldest) const
{
    // index 0 is the oldest sample held, which is the one just ahead of the write head
    const auto position = writePosition.load(std::memory_order_acquire);
    const auto wrapped = (position + juce::jlimit(0, historySize - 1, indexFromOldest)) % historySize;

    return history.getSample(juce::jlimit(0, 1, channel), wrapped);
}

void ScopeSampleBuffer::readHistory(std::vector<float>& outLeft, std::vector<float>& outRight) const
{
    outLeft.resize((size_t) historySize);
    outRight.resize((size_t) historySize);

    // unwrap the ring so the caller always gets oldest first, newest last
    const auto position = writePosition.load(std::memory_order_acquire);
    const auto tail = historySize - position;

    std::memcpy(outLeft.data(), history.getReadPointer(0, position), sizeof(float) * (size_t) tail);
    std::memcpy(outRight.data(), history.getReadPointer(1, position), sizeof(float) * (size_t) tail);

    if (position > 0)
    {
        std::memcpy(outLeft.data() + tail, history.getReadPointer(0, 0), sizeof(float) * (size_t) position);
        std::memcpy(outRight.data() + tail, history.getReadPointer(1, 0), sizeof(float) * (size_t) position);
    }
}

//==============================================================================

ScopeDisplay::ScopeDisplay(const ScopeSampleBuffer& bufferToRead, juce::Colour leftColour,
                           juce::Colour rightColour)
    : buffer(bufferToRead), leftTrace(leftColour), rightTrace(rightColour)
{
    setInterceptsMouseClicks(true, false);
    startTimerHz(50);
}

void ScopeDisplay::setView(float newHorizontalZoom, float newVerticalZoom)
{
    horizontalZoom = juce::jlimit(minimumHorizontalZoom, maximumHorizontalZoom, newHorizontalZoom);
    verticalZoom = juce::jlimit(minimumVerticalZoom, maximumVerticalZoom, newVerticalZoom);
    clampPan();
    repaint();
}

void ScopeDisplay::setStereo(bool shouldCombineChannels)
{
    stereo = shouldCombineChannels;
    repaint();
}

void ScopeDisplay::setFrozen(bool shouldFreeze)
{
    // take a copy of everything the ring holds, so the held view can still be zoomed out
    if (shouldFreeze && ! frozen)
        buffer.readHistory(snapshotLeft, snapshotRight);

    frozen = shouldFreeze;
    repaint();
}

void ScopeDisplay::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
}

void ScopeDisplay::resetView()
{
    panSamples = 0.0f;
    panPixelsY = 0.0f;
    repaint();
}

void ScopeDisplay::timerCallback()
{
    // Live rendering reads the ring directly, so there is nothing to copy here; a frozen scope
    // renders the snapshot taken when it was frozen.
    if (! frozen)
        repaint();
}

float ScopeDisplay::sampleAt(int channel, int index) const
{
    if (frozen)
    {
        const auto& samples = channel == 1 ? snapshotRight : snapshotLeft;

        if (samples.empty())
            return 0.0f;

        const auto clamped = (size_t) juce::jlimit(0, (int) samples.size() - 1, index);

        if (channel == 2)
            return 0.5f * (snapshotLeft[clamped]
                           + (snapshotRight.empty() ? 0.0f : snapshotRight[clamped]));

        return samples[clamped];
    }

    if (channel == 2)
        return 0.5f * (buffer.getSample(0, index) + buffer.getSample(1, index));

    return buffer.getSample(channel, index);
}

//==============================================================================
// view

float ScopeDisplay::getSamplesPerPixel() const
{
    // same feel as the previous renderer: the H Zoom control spans 1..128 samples per column
    return 1.0f + 127.0f * juce::jlimit(0.0f, 1.0f, horizontalZoom);
}

int ScopeDisplay::getVisibleSampleCount() const
{
    return juce::jmax(2, juce::roundToInt((float) getWidth() * getSamplesPerPixel()));
}

int ScopeDisplay::getMaximumPanSamples() const
{
    return juce::jmax(0, buffer.getHistorySize() - getVisibleSampleCount());
}

float ScopeDisplay::getMaximumPanY() const
{
    const auto laneHeight = (float) getHeight() * (stereo ? 1.0f : 0.5f);

    return juce::jmax(0.0f, laneHeight * (verticalZoom - 1.0f) * 0.5f);
}

bool ScopeDisplay::isPannable() const
{
    return getMaximumPanSamples() > 0 || getMaximumPanY() > 0.0f;
}

void ScopeDisplay::clampPan()
{
    panSamples = juce::jlimit(0.0f, (float) getMaximumPanSamples(), panSamples);
    panPixelsY = juce::jlimit(-getMaximumPanY(), getMaximumPanY(), panPixelsY);
}

int ScopeDisplay::findTriggerOffset(int channel, int windowStart) const
{
    // look back up to a quarter of the window for the most recent rising zero crossing
    const auto searchLength = juce::jmin(windowStart, getVisibleSampleCount() / 4);

    for (int offset = 0; offset < searchLength; ++offset)
    {
        const auto index = windowStart - offset;

        if (index <= 0)
            break;

        if (sampleAt(channel, index - 1) <= 0.0f && sampleAt(channel, index) > 0.0f)
            return index;
    }

    return windowStart;
}

//==============================================================================
// painting

void ScopeDisplay::paint(juce::Graphics& g)
{
    using namespace juce;

    const auto bounds = getLocalBounds().toFloat();

    if (bounds.getWidth() < 4.0f || bounds.getHeight() < 4.0f)
        return;

    g.fillAll(Colours::black);

    constexpr int fontHeight = 10;
    g.setFont(Font("Segoe UI", (float) fontHeight, Font::plain));

    drawTimeAxis(g, bounds);

    if (stereo)
    {
        drawLane(g, bounds, 2, leftTrace, fontHeight);
    }
    else
    {
        auto remaining = bounds;
        const auto upper = remaining.removeFromTop(bounds.getHeight() * 0.5f);

        drawLane(g, upper, 0, leftTrace, fontHeight);
        drawLane(g, remaining, 1, rightTrace, fontHeight);

        g.setColour(BiztortionTheme::border.withAlpha(0.6f));
        g.drawHorizontalLine(roundToInt(upper.getBottom()), bounds.getX(), bounds.getRight());
    }

    // channel legend, matching the colours the L / R switch uses
    g.setColour(stereo ? leftTrace : leftTrace);
    g.drawText(stereo ? "Stereo" : "L", bounds.reduced(8.0f, 5.0f).getSmallestIntegerContainer(),
               Justification::topLeft, false);

    if (frozen)
    {
        g.setColour(BiztortionTheme::text.withAlpha(0.7f));
        g.drawText("frozen", bounds.reduced(8.0f, 5.0f).getSmallestIntegerContainer(),
                   Justification::topRight, false);
    }
}

void ScopeDisplay::drawTimeAxis(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    using namespace juce;

    constexpr int divisions = 4;

    for (int division = 1; division < divisions; ++division)
    {
        const auto x = bounds.getX() + bounds.getWidth() * (float) division / (float) divisions;

        g.setColour(BiztortionTheme::border.withAlpha(0.35f));
        g.drawVerticalLine(roundToInt(x), bounds.getY(), bounds.getBottom());
    }

    const auto windowMs = 1000.0 * (double) getVisibleSampleCount() / sampleRate;

    g.setColour(BiztortionTheme::textMuted);
    g.drawText(windowMs < 10.0 ? String(windowMs, 2) + " ms" : String(windowMs, 1) + " ms",
               bounds.reduced(6.0f, 3.0f).getSmallestIntegerContainer(),
               Justification::bottomLeft, false);
}

void ScopeDisplay::drawLane(juce::Graphics& g, juce::Rectangle<float> lane,
                            int channel, juce::Colour colour, int fontHeight)
{
    using namespace juce;

    const auto centreY = lane.getCentreY() + panPixelsY;
    const auto halfHeight = lane.getHeight() * 0.5f;
    const auto zoom = jmax(minimumVerticalZoom, verticalZoom);

    const auto amplitudeToY = [centreY, halfHeight, zoom](float amplitude) {
        return centreY - halfHeight * zoom * amplitude;
    };

    // --- amplitude references, generated from the range the current zoom makes visible ---
    g.setColour(BiztortionTheme::border.brighter(0.15f).withAlpha(0.8f));
    g.drawHorizontalLine(roundToInt(centreY), lane.getX(), lane.getRight());

    const auto visibleMax = 1.0f / zoom;
    const auto rawStep = visibleMax / 4.0f;
    const auto magnitude = std::pow(10.0f, std::floor(std::log10(jmax(1.0e-4f, rawStep))));
    const auto normalised = rawStep / magnitude;
    const auto step = magnitude * (normalised < 1.5f ? 1.0f : (normalised < 3.5f ? 2.0f : 5.0f));

    for (auto amplitude = step; amplitude <= visibleMax + step * 0.01f; amplitude += step)
    {
        for (const auto signedAmplitude : { -amplitude, amplitude })
        {
            const auto y = amplitudeToY(signedAmplitude);

            if (y < lane.getY() + 1.0f || y > lane.getBottom() - 1.0f)
                continue;

            const auto isFullScale = std::abs(std::abs(signedAmplitude) - 1.0f) < step * 0.01f;

            g.setColour(BiztortionTheme::border.withAlpha(isFullScale ? 0.55f : 0.28f));
            g.drawHorizontalLine(roundToInt(y), lane.getX(), lane.getRight());

            String label;
            if (signedAmplitude > 0.0f)
                label << "+";
            label << String(signedAmplitude, step < 1.0f ? (step < 0.1f ? 2 : 1) : 0);

            g.setColour(isFullScale ? BiztortionTheme::text.withAlpha(0.75f)
                                    : BiztortionTheme::textMuted);
            g.drawText(label,
                       Rectangle<float>(lane.getRight() - 38.0f, y - (float) fontHeight * 0.5f - 1.0f,
                                        34.0f, (float) fontHeight + 2.0f).getSmallestIntegerContainer(),
                       Justification::centredRight, false);
        }
    }

    // --- trace ---
    const auto total = buffer.getHistorySize();
    // never ask for more samples than the history holds, so zooming right out keeps drawing
    const auto visible = jmin(total - 1, getVisibleSampleCount());

    auto windowStart = total - visible - roundToInt(panSamples);
    windowStart = jlimit(0, jmax(0, total - visible - 1), windowStart);
    // sync is always on: line the window up with a rising zero crossing
    windowStart = findTriggerOffset(channel, windowStart);

    const auto columns = jmax(1, (int) lane.getWidth());
    // the window may have been clipped to the history, so derive the span from what is shown
    const auto columnSpan = (float) visible / (float) columns;

    // one min/max pair per pixel column, so the trace stays honest however far it is zoomed out
    std::vector<float> topEdge((size_t) columns), bottomEdge((size_t) columns);

    for (int column = 0; column < columns; ++column)
    {
        const auto first = windowStart + roundToInt((float) column * columnSpan);
        const auto last = jmin(total - 1, first + jmax(1, roundToInt(columnSpan)));

        auto minimum = sampleAt(channel, jlimit(0, total - 1, first));
        auto maximum = minimum;

        // reading every sample of a very wide window would cost more than it shows, so step
        // through each column with a bounded number of probes
        const auto stride = jmax(1, (last - first) / 64);

        for (auto index = first; index <= last; index += stride)
        {
            const auto value = sampleAt(channel, jlimit(0, total - 1, index));
            minimum = jmin(minimum, value);
            maximum = jmax(maximum, value);
        }

        topEdge[(size_t) column] = jlimit(lane.getY(), lane.getBottom(), amplitudeToY(maximum));
        bottomEdge[(size_t) column] = jlimit(lane.getY(), lane.getBottom(), amplitudeToY(minimum));
    }

    // closed envelope: along the maxima, then back along the minima
    Path envelope;
    envelope.startNewSubPath(lane.getX(), topEdge[0]);

    for (int column = 1; column < columns; ++column)
        envelope.lineTo(lane.getX() + (float) column, topEdge[(size_t) column]);

    for (int column = columns - 1; column >= 0; --column)
        envelope.lineTo(lane.getX() + (float) column, bottomEdge[(size_t) column]);

    envelope.closeSubPath();

    g.setColour(colour.withAlpha(0.16f));
    g.fillPath(envelope);

    g.setColour(colour);
    g.strokePath(envelope, PathStrokeType(1.2f, PathStrokeType::curved, PathStrokeType::rounded));
}

//==============================================================================
// gestures

void ScopeDisplay::updateViewCursor(bool dragging)
{
    setMouseCursor(isPannable() ? (dragging ? juce::MouseCursor::DraggingHandCursor
                                            : juce::MouseCursor::PointingHandCursor)
                                : juce::MouseCursor::NormalCursor);
    updateMouseCursor();
}

void ScopeDisplay::reportZoom(float newHorizontalZoom, float newVerticalZoom)
{
    const auto clampedHorizontal = juce::jlimit(minimumHorizontalZoom, maximumHorizontalZoom,
                                                newHorizontalZoom);
    const auto clampedVertical = juce::jlimit(minimumVerticalZoom, maximumVerticalZoom,
                                              newVerticalZoom);

    if (onZoomGesture != nullptr)
        onZoomGesture(clampedHorizontal, clampedVertical);
}

void ScopeDisplay::mouseEnter(const juce::MouseEvent&)   { updateViewCursor(false); }
void ScopeDisplay::mouseMove(const juce::MouseEvent&)    { updateViewCursor(false); }
void ScopeDisplay::mouseExit(const juce::MouseEvent&)    { setMouseCursor(juce::MouseCursor::NormalCursor); }

void ScopeDisplay::mouseDown(const juce::MouseEvent&)
{
    panSamplesAtDragStart = panSamples;
    panPixelsYAtDragStart = panPixelsY;
    updateViewCursor(true);
}

void ScopeDisplay::mouseDrag(const juce::MouseEvent& event)
{
    if (! isPannable())
        return;

    // dragging right travels back in time, like scrolling a waveform
    panSamples = panSamplesAtDragStart
        + (float) event.getDistanceFromDragStartX() * getSamplesPerPixel();
    panPixelsY = panPixelsYAtDragStart + (float) event.getDistanceFromDragStartY();
    clampPan();
    repaint();
}

void ScopeDisplay::mouseUp(const juce::MouseEvent&)
{
    updateViewCursor(false);
}

void ScopeDisplay::mouseDoubleClick(const juce::MouseEvent&)
{
    resetView();
}

void ScopeDisplay::mouseWheelMove(const juce::MouseEvent& event,
                                  const juce::MouseWheelDetails& wheel)
{
    const auto direction = wheel.isReversed ? -1.0f : 1.0f;

    // sideways two finger scroll travels along the time axis
    if (! juce::approximatelyEqual(wheel.deltaX, 0.0f))
    {
        panSamples += wheel.deltaX * direction * (float) getVisibleSampleCount() * 0.5f;
        clampPan();
        repaint();
        return;
    }

    if (juce::approximatelyEqual(wheel.deltaY, 0.0f))
    {
        juce::Component::mouseWheelMove(event, wheel);
        return;
    }

    // a mouse has no pinch gesture, so a modifier switches the wheel to the time axis
    if (event.mods.isCommandDown() || event.mods.isShiftDown() || event.mods.isAltDown())
    {
        reportZoom(horizontalZoom - wheel.deltaY * direction * 0.25f, verticalZoom);
        return;
    }

    reportZoom(horizontalZoom, verticalZoom * (1.0f + wheel.deltaY * direction * 0.4f));
}

void ScopeDisplay::mouseMagnify(const juce::MouseEvent&, float scaleFactor)
{
    // pinching out shows less time, i.e. a smaller horizontal zoom value
    reportZoom(horizontalZoom / juce::jmax(0.1f, scaleFactor), verticalZoom);
}
