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

#pragma once

#include <JuceHeader.h>

#include "../Shared/PluginState.h"

using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, CutFilter>;

//==============================================================================

/* ResponseCurveComponent */

//==============================================================================

struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer {

    ResponseCurveComponent(PluginState& p, unsigned int parameterNumber);
    ~ResponseCurveComponent();

    /** Receives a callback when a parameter has been changed.

            IMPORTANT NOTE: This will be called synchronously when a parameter changes, and
            many audio processors will change their parameter during their audio callback.
            This means that not only has your handler code got to be completely thread-safe,
            but it's also got to be VERY fast, and avoid blocking. If you need to handle
            this event on your message thread, use this callback to trigger an AsyncUpdater
            or ChangeBroadcaster which you can respond to on the message thread.
        */
    void parameterValueChanged(int parameterIndex, float newValue) override;

    /** Indicates that a parameter change gesture has started.

        E.g. if the user is dragging a slider, this would be called with gestureIsStarting
        being true when they first press the mouse button, and it will be called again with
        gestureIsStarting being false when they release it.

        IMPORTANT NOTE: This will be called synchronously, and many audio processors will
        call it during their audio callback. This means that not only has your handler code
        got to be completely thread-safe, but it's also got to be VERY fast, and avoid
        blocking. If you need to handle this event on your message thread, use this callback
        to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
        message thread.
    */
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }
    /** The user-defined callback routine that actually gets called periodically.

        It's perfectly ok to call startTimer() or stopTimer() from within this
        callback to change the subsequent intervals.
    */
    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Interactive band handles, like any parametric equalizer: drag to change frequency (and
    // gain, for the peaks), double click to bypass that band.
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    // wheel over a handle changes that band's Q (peaks) or slope (cuts)
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:

    PluginState& pluginState;

    juce::Atomic<bool> parameterChanged{ false };
    MonoChain monoChain;
    juce::Path responseCurve;

    unsigned int parameterNumber;

    void updateResponseCurve();
    void updateChain();

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysysArea();

    //==============================================================================
    // band handles

    enum class Band { none, lowCut, peak1, peak2, highCut };

    struct BandHandle
    {
        Band band = Band::none;
        juce::Point<float> position;
        bool bypassed = false;
        // only the peaks are draggable vertically
        bool hasGain = false;
    };

    // gain range the response curve is drawn over (matches the equalizer's gain scale)
    static constexpr float maximumGainDb = 24.0f;
    // click radius around a handle, in pixels
    static constexpr float handleHitRadius = 11.0f;
    static constexpr float handleRadius = 6.0f;

    // mirrors the module bypass: the handles are drawn inert and stop responding
    bool moduleBypassed = false;

    std::vector<BandHandle> getBandHandles();
    Band findBandAt(juce::Point<float> position);
    juce::String getBandParameterID(Band band, const juce::String& parameter) const;
    juce::RangedAudioParameter* getBandParameter(Band band, const juce::String& parameter) const;

    float frequencyToX(float frequency);
    float xToFrequency(float x);
    float gainToY(float gainDb);
    float yToGainDb(float y);

    void setBandParameterValue(Band band, const juce::String& parameter, float value);

    Band draggedBand = Band::none;
    Band hoveredBand = Band::none;
};