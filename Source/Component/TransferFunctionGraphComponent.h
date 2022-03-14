/*
  ==============================================================================

	TransferFunctionGraphComponent.h

	Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

	Copyright (c) 2018 Daniele Filaretti
	Content: the Transfer Function Graph Component
	Source: https://github.com/dfilaretti/waveshaper-demo

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

class TransferFunctionGraphComponent : public juce::Component,
	juce::AudioProcessorParameter::Listener,
	juce::Timer
{
public:
	//==============================================================================
	TransferFunctionGraphComponent(PluginState& p, unsigned int parameterNumber);
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

	PluginState& pluginState;

	juce::Atomic<bool> parameterChanged{ false };
	unsigned int parameterNumber;

	void updateParams();
	void setTanhAmp(float v);
	void setTanhSlope(float v);
	void setSineAmp(float v);
	void setSineFreq(float v);

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransferFunctionGraphComponent)
};