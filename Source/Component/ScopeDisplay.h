/*
  ==============================================================================

    ScopeDisplay.h

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

#pragma once

#include <JuceHeader.h>

#include <functional>
#include <vector>

//==============================================================================

/**
    Lock free ring of raw samples shared between the audio thread and the scope.

    The oscilloscope keeps the samples themselves rather than a pre-rendered image, which is
    what lets the display be re-drawn at a different zoom after it has been frozen.
*/
class ScopeSampleBuffer
{
public:
    ScopeSampleBuffer();

    // audio thread
    void push(const float* left, const float* right, int numSamples);

    // message thread: reads straight out of the ring, index 0 = oldest sample held.
    // The scope renders from here every frame, so nothing is copied while it runs live.
    float getSample(int channel, int indexFromOldest) const;

    // message thread: copies the whole history, oldest first (used when freezing)
    void readHistory(std::vector<float>& outLeft, std::vector<float>& outRight) const;

    int getHistorySize() const noexcept { return historySize; }

private:
    // ~5.9 s at 44.1 kHz, so the widest time window still has samples to show. Nothing copies
    // this per frame, so the size only costs memory (2 MB).
    static constexpr int historySize = 1 << 18;

    juce::AudioBuffer<float> history;
    std::atomic<int> writePosition { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScopeSampleBuffer)
};

//==============================================================================

/**
    The oscilloscope graph: traces, amplitude/time references and the gestures that go with
    them, all reading from a ScopeSampleBuffer.

    Because it renders from the samples every frame, freezing simply stops taking new snapshots:
    zooming or panning afterwards re-draws the held samples at the new view.
*/
class ScopeDisplay : public juce::Component,
                     public juce::SettableTooltipClient,
                     private juce::Timer
{
public:
    ScopeDisplay(const ScopeSampleBuffer& bufferToRead, juce::Colour leftColour,
                 juce::Colour rightColour);

    /** @param horizontalZoom  0..1, the module's H Zoom parameter (0 = shortest time window)
        @param verticalZoom    0..2, the module's V Zoom parameter (1 = full scale is +/-1)
    */
    void setView(float horizontalZoom, float verticalZoom);
    void setStereo(bool shouldCombineChannels);
    // holds the current samples on screen; the view can still be zoomed and panned
    void setFrozen(bool shouldFreeze);
    void setSampleRate(double newSampleRate);
    void resetView();

    // reports a zoom performed on the graph so the module's knobs can follow
    std::function<void(float horizontalZoom, float verticalZoom)> onZoomGesture;

    void paint(juce::Graphics& g) override;

    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify(const juce::MouseEvent& event, float scaleFactor) override;

    static constexpr float minimumHorizontalZoom = 0.0f;
    static constexpr float maximumHorizontalZoom = 1.0f;
    static constexpr float minimumVerticalZoom = 0.05f;
    static constexpr float maximumVerticalZoom = 2.0f;

private:
    void timerCallback() override;

    // channel: 0 = left, 1 = right, 2 = the average of both
    void drawLane(juce::Graphics& g, juce::Rectangle<float> lane, int channel,
                  juce::Colour colour, int fontHeight);
    float sampleAt(int channel, int index) const;
    void drawTimeAxis(juce::Graphics& g, juce::Rectangle<float> bounds);

    float getSamplesPerPixel() const;
    int getVisibleSampleCount() const;
    int getMaximumPanSamples() const;
    float getMaximumPanY() const;
    bool isPannable() const;
    void clampPan();
    void updateViewCursor(bool dragging);
    void reportZoom(float horizontalZoom, float verticalZoom);
    // aligns the window to a rising zero crossing so repeating shapes stand still
    int findTriggerOffset(int channel, int windowStart) const;

    const ScopeSampleBuffer& buffer;
    juce::Colour leftTrace, rightTrace;

    // only filled while frozen: the held samples the view keeps re-rendering
    std::vector<float> snapshotLeft, snapshotRight;

    float horizontalZoom = 0.0f;
    float verticalZoom = 1.0f;
    bool stereo = true;
    bool frozen = false;
    double sampleRate = 44100.0;

    // how far back from the newest sample the window starts, and the vertical offset in pixels
    float panSamples = 0.0f;
    float panPixelsY = 0.0f;
    float panSamplesAtDragStart = 0.0f;
    float panPixelsYAtDragStart = 0.0f;
};
