/*
  ==============================================================================

    FFTAnalyzerComponent.h
    Created: 24 Aug 2021 5:21:17pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

struct FFTAnalyzerComponent : juce::Component,
    juce::AudioProcessorParameter::Listener,
    juce::Timer {

    FFTAnalyzerComponent(BiztortionAudioProcessor&);
    ~FFTAnalyzerComponent();
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
    void monoChainUpdate();
    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    BiztortionAudioProcessor& audioProcessor;
    juce::Image background;

    // for the FFT analyzer
    PathProducer leftPathProducer, rightPathProducer;

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysysArea();
};