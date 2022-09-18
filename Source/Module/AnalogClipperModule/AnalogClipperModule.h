/*
  ==============================================================================

	AnalogClipperModule.h

	Copyright (c) 2021 KillBizz - Gabriel Bizzo e Francesco Magoga

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

#ifndef AnalogClipperModule_h
#define AnalogClipperModule_h

#include <JuceHeader.h>

#include "../../Shared/GUIStuff.h"
#include "../../Component/TransferFunctionGraphComponent.h"
#include "../../Module/DSPModule.h"
#include "../../Module/GUIModule.h"
#include "../../Shared/PluginState.h"

#include "Clipper.h"

struct AnalogClipperSettings
{
	float mix{ 0 }, gain{ 0 };
	bool bypassed{ false };
};

class AnalogClipperModuleDSP : public DSPModule
{
	public:
		AnalogClipperModuleDSP(juce::AudioProcessorValueTreeState& _apvts);
		virtual ~AnalogClipperModuleDSP();

		void updateDSPState(double sampleRate) override;
		void prepareToPlay(double sampleRate, int samplesPerBlock) override;
		void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate) override;

		static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout&);
		static AnalogClipperSettings getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber);

	private:
		Clipper clippers[2];

		bool bypassed = false;
		juce::AudioBuffer<float> wetBuffer, tempBuffer;
		juce::LinearSmoothedValue<float> gainGain, dryGain, wetGain;

		/*static const size_t numChannels = 2;
		static const size_t oversamplingOrder = 4;
		static const int    oversamplingFactor = 1 << oversamplingOrder;
		static const auto filterType = juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR;

		juce::dsp::Oversampling<float> oversampler{ numChannels, oversamplingOrder, filterType };*/
};

//==============================================================================

/* WaveshaperModule GUI */

//==============================================================================

class AnalogClipperModuleGUI : public GUIModule
{
	public:
		AnalogClipperModuleGUI(PluginState& p, unsigned int parameterNumber);
		virtual ~AnalogClipperModuleGUI();

		std::vector<juce::Component*> getAllComps() override;
		std::vector<juce::Component*> getParamComps() override;
		virtual void updateParameters(const juce::Array<juce::var>& values) override;
		virtual void resetParameters(unsigned int parameterNumber) override;
		virtual juce::Array<juce::var> getParamValues() override;

		void resized() override;

	private:

		using APVTS = juce::AudioProcessorValueTreeState;
		using Attachment = APVTS::SliderAttachment;
		using ButtonAttachment = APVTS::ButtonAttachment;

		PluginState& pluginState;

		juce::Label title;

		TransferFunctionGraphComponent transferFunctionGraph;

		juce::Label gainLabel, mixLabel;
		RotarySliderWithLabels gainSlider, mixSlider;
		Attachment gainSliderAttachment, mixSliderAttachment;

		PowerButton bypassButton;

		ButtonAttachment bypassButtonAttachment;

		ButtonsLookAndFeel lnf;
};

#endif /* AnalogClipperModule_h */
