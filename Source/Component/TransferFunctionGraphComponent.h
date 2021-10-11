/*
  ==============================================================================

    TransferFunctionGraphComponent.h
    Created: 28 Aug 2021 6:53:09pm
    Author:  gabri

  ==============================================================================
*/

/*
  ==============================================================================

	CREDITS for the Transfer Function Graph
	Author: Daniele Filaretti
	Source: https://github.com/dfilaretti/waveshaper-demo

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
class BiztortionAudioProcessor;

class TransferFunctionGraphComponent : public juce::Component,
	juce::AudioProcessorParameter::Listener,
	juce::Timer
{
public:
	//==============================================================================
	TransferFunctionGraphComponent(BiztortionAudioProcessor& p, unsigned int chainPosition);
	~TransferFunctionGraphComponent();

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

	void resized() override
	{
		// This is called when the MainContentComponent is resized.
		// If you add any child components, this is where you should
		// update their positions.
	}

private:
	// params
	float tanhAmplitude, tanhSlope, sineAmplitude, sineFrequency;
	BiztortionAudioProcessor& audioProcessor;
	juce::Atomic<bool> parameterChanged{ false };
	unsigned int chainPosition;

	void updateParams();
	void setTanhAmp(float v);
	void setTanhSlope(float v);
	void setSineAmp(float v);
	void setSineFreq(float v);

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransferFunctionGraphComponent)
};