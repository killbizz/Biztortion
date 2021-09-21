/*
  ==============================================================================

    ResponseCurveComponent.h
    Created: 17 Aug 2021 6:24:03pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
class BiztortionAudioProcessor;

using Filter = juce::dsp::IIR::Filter<float>;
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

//==============================================================================

/* ResponseCurveComponent */

//==============================================================================

struct ResponseCurveComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer {

    ResponseCurveComponent(BiztortionAudioProcessor& p, unsigned int chainPosition);
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
    //void monoChainUpdate();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setFilterMonoChain();

private:

    BiztortionAudioProcessor& audioProcessor;
    juce::Atomic<bool> parameterChanged{ false };
    MonoChain* filterMonoChain = nullptr;
    unsigned int chainPosition;

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysysArea();
};