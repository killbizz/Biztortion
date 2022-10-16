/*
  ==============================================================================

	AnalogClipperModule.cpp

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

#include "AnalogClipperModule.h"

//==============================================================================

/* AnalogClipperModule DSP */

//==============================================================================

AnalogClipperModuleDSP::AnalogClipperModuleDSP(juce::AudioProcessorValueTreeState& _apvts)
	: DSPModule(_apvts)
{
}

AnalogClipperModuleDSP::~AnalogClipperModuleDSP()
{
	
}

void AnalogClipperModuleDSP::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	for (auto channel = 0; channel < 2; channel++)
		clippers[channel].setSampleRate(sampleRate);
}

void AnalogClipperModuleDSP::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, double sampleRate)
{

	updateDSPState(sampleRate);

	if (!bypassed) {

		// SAFETY CHECK :::: since some hosts will change buffer sizes without calling prepToPlay (ex: Bitwig)
		int numSamples = buffer.getNumSamples();
		if (wetBuffer.getNumSamples() != numSamples)
		{
			wetBuffer.setSize(2, numSamples, false, true, true); // clears
			tempBuffer.setSize(2, numSamples, false, true, true); // clears
		}
		
		// Wet Buffer feeding
		for (auto channel = 0; channel < 2; channel++)
			wetBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

		// Oversampling wetBuffer for processing
		/*juce::dsp::AudioBlock<float> block(wetBuffer);
		auto leftBlock = block.getSingleChannelBlock(0);
		auto rightBlock = block.getSingleChannelBlock(1);
		juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
		juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
		auto oversampledLeftBlock = oversampler.processSamplesUp(leftContext.getInputBlock());
		auto oversampledRightBlock = oversampler.processSamplesUp(rightContext.getInputBlock());*/

		// Gain
		gainGain.applyGain(wetBuffer, numSamples);

		// Temp Buffer feeding for applying asymmetry
		for (auto channel = 0; channel < 2; channel++)
			tempBuffer.copyFrom(channel, 0, wetBuffer, channel, 0, numSamples);

		// Analog Clipper
		for (auto channel = 0; channel < 2; channel++)
		{
			auto* channelData = wetBuffer.getWritePointer(channel);

			clippers[channel].process(channelData, numSamples);
		}
		
		// Volume
		volumeValue.applyGain(wetBuffer, numSamples);

		// Sampling back down the wetBuffer after processing
		/*oversampler.processSamplesDown(leftContext.getOutputBlock());
		oversampler.processSamplesDown(rightContext.getOutputBlock());

		auto* channelData = wetBuffer.getWritePointer(0);
		for (auto i = 0; i < numSamples; i++)
			channelData[i] = leftContext.getOutputBlock().getSample(0, i);
		channelData = wetBuffer.getWritePointer(1);
		for (auto i = 0; i < numSamples; i++)
			channelData[i] = rightContext.getOutputBlock().getSample(0, i);*/
		
		// Mixing buffers
		dryGain.applyGain(buffer, numSamples);
		wetGain.applyGain(wetBuffer, numSamples);

		for (auto channel = 0; channel < 2; channel++)
			buffer.addFrom(channel, 0, wetBuffer, channel, 0, numSamples);

		// Hard clipper for limiting
		for (auto channel = 0; channel < 2; channel++)
		{
			auto* channelData = buffer.getWritePointer(channel);

			for (auto i = 0; i < numSamples; i++)
				channelData[i] = juce::jlimit(-1.f, 1.f, channelData[i]);
		}
	}
}

void AnalogClipperModuleDSP::addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
{
	using namespace juce;

	for (int i = 1; i < 9; ++i) {
		layout.add(std::make_unique<juce::AudioParameterFloat>("Analog Clipper Gain " + std::to_string(i), "Analog Clipper Gain " + std::to_string(i), NormalisableRange<float>(0.f, 40.f, 0.01f), 0.f, "Analog Clipper " + std::to_string(i)));
		layout.add(std::make_unique<AudioParameterFloat>("Analog Clipper Mix " + std::to_string(i), "Analog Clipper Mix " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Analog Clipper " + std::to_string(i)));
		layout.add(std::make_unique<AudioParameterFloat>("Analog Clipper Volume " + std::to_string(i), "Analog Clipper Volume " + std::to_string(i), NormalisableRange<float>(0.f, 100.f, 0.01f), 100.f, "Analog Clipper " + std::to_string(i)));
		layout.add(std::make_unique<AudioParameterBool>("Analog Clipper Bypassed " + std::to_string(i), "Analog Clipper Bypassed " + std::to_string(i), false, "Analog Clipper " + std::to_string(i)));
	}
}

AnalogClipperSettings AnalogClipperModuleDSP::getSettings(juce::AudioProcessorValueTreeState& apvts, unsigned int parameterNumber)
{
	AnalogClipperSettings settings;

	settings.gain = apvts.getRawParameterValue("Analog Clipper Gain " + std::to_string(parameterNumber))->load();
	settings.mix = apvts.getRawParameterValue("Analog Clipper Mix " + std::to_string(parameterNumber))->load();
	settings.volume = apvts.getRawParameterValue("Analog Clipper Volume " + std::to_string(parameterNumber))->load();
	settings.bypassed = apvts.getRawParameterValue("Analog Clipper Bypassed " + std::to_string(parameterNumber))->load() > 0.5f;

	return settings;
}

void AnalogClipperModuleDSP::updateDSPState(double)
{
	auto settings = getSettings(apvts, parameterNumber);

	bypassed = settings.bypassed;

	auto mix = settings.mix * 0.01f;
	dryGain.setTargetValue(1.f - mix);
	wetGain.setTargetValue(mix);
	gainGain.setTargetValue(juce::Decibels::decibelsToGain(settings.gain));
	volumeValue.setTargetValue(settings.volume * 0.01f);
}

//==============================================================================

/* AnalogClipperModule GUI */

//==============================================================================

AnalogClipperModuleGUI::AnalogClipperModuleGUI(PluginState& p, unsigned int parameterNumber)
	: GUIModule(), pluginState(p),
	gainSlider(*pluginState.apvts.getParameter("Analog Clipper Gain " + std::to_string(parameterNumber)), "dB"),
	mixSlider(*pluginState.apvts.getParameter("Analog Clipper Mix " + std::to_string(parameterNumber)), "%"),
	volumeSlider(*pluginState.apvts.getParameter("Analog Clipper Volume " + std::to_string(parameterNumber)), "%"),
	transferFunctionGraph(p, parameterNumber),
	gainSliderAttachment(pluginState.apvts, "Analog Clipper Gain " + std::to_string(parameterNumber), gainSlider),
	mixSliderAttachment(pluginState.apvts, "Analog Clipper Mix " + std::to_string(parameterNumber), mixSlider),
	volumeSliderAttachment(pluginState.apvts, "Analog Clipper Volume " + std::to_string(parameterNumber), volumeSlider),
	bypassButtonAttachment(pluginState.apvts, "Analog Clipper Bypassed " + std::to_string(parameterNumber), bypassButton)
{
	// title setup
	title.setText("Analog Clipper", juce::dontSendNotification);
	title.setFont(ModuleLookAndFeel::getTitlesFont());

	// labels
	gainLabel.setText("Gain", juce::dontSendNotification);
	gainLabel.setFont(ModuleLookAndFeel::getLabelsFont());
	mixLabel.setText("Mix", juce::dontSendNotification);
	mixLabel.setFont(ModuleLookAndFeel::getLabelsFont());
	volumeLabel.setText("Volume", juce::dontSendNotification);
	volumeLabel.setFont(ModuleLookAndFeel::getLabelsFont());

	gainSlider.labels.add({ 0.f, "0dB" });
	gainSlider.labels.add({ 1.f, "40dB" });
	mixSlider.labels.add({ 0.f, "0%" });
	mixSlider.labels.add({ 1.f, "100%" });
	volumeSlider.labels.add({ 0.f, "0%" });
	volumeSlider.labels.add({ 1.f, "100%" });

	bypassButton.setLookAndFeel(&lnf);

	auto safePtr = juce::Component::SafePointer<AnalogClipperModuleGUI>(this);
	bypassButton.onClick = [safePtr]()
	{
		if (auto* comp = safePtr.getComponent())
		{
			auto bypassed = comp->bypassButton.getToggleState();
			comp->handleParamCompsEnablement(bypassed);
		}
	};

	// tooltips
	bypassButton.setTooltip("Bypass this module");
	gainSlider.setTooltip("Select the amount of gain to be applied to the module input signal");
	mixSlider.setTooltip("Select the blend between the unprocessed and processed signal");
	volumeSlider.setTooltip("Select the volume of the processed signal");

	for (auto* comp : getAllComps())
	{
		addAndMakeVisible(comp);
	}

	handleParamCompsEnablement(bypassButton.getToggleState());
}

AnalogClipperModuleGUI::~AnalogClipperModuleGUI()
{
	bypassButton.setLookAndFeel(nullptr);
}

std::vector<juce::Component*> AnalogClipperModuleGUI::getAllComps()
{
	return {
		&title,
		&transferFunctionGraph,
		&gainSlider,
		&mixSlider,
		&volumeSlider,
		// labels
		&gainLabel,
		&mixLabel,
		&volumeLabel,
		// bypass
		&bypassButton
	};
}

std::vector<juce::Component*> AnalogClipperModuleGUI::getParamComps()
{
	return {
		&gainSlider,
		&mixSlider,
		&volumeSlider
	};
}

void AnalogClipperModuleGUI::updateParameters(const juce::Array<juce::var>& values)
{
	auto value = values.begin();

	bypassButton.setToggleState(*(value++), juce::NotificationType::sendNotificationSync);
	gainSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
	mixSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
	volumeSlider.setValue(*(value++), juce::NotificationType::sendNotificationSync);
}

void AnalogClipperModuleGUI::resetParameters(unsigned int parameterNumber)
{
	auto gain = pluginState.apvts.getParameter("Analog Clipper Gain " + std::to_string(parameterNumber));
	auto mix = pluginState.apvts.getParameter("Analog Clipper Mix " + std::to_string(parameterNumber));
	auto volume = pluginState.apvts.getParameter("Analog Clipper Volume " + std::to_string(parameterNumber));
	auto bypassed = pluginState.apvts.getParameter("Analog Clipper Bypassed " + std::to_string(parameterNumber));

	gain->setValueNotifyingHost(gain->getDefaultValue());
	mix->setValueNotifyingHost(mix->getDefaultValue());
	volume->setValueNotifyingHost(volume->getDefaultValue());
	bypassed->setValueNotifyingHost(bypassed->getDefaultValue());
}

juce::Array<juce::var> AnalogClipperModuleGUI::getParamValues()
{
	juce::Array<juce::var> values;

	values.add(juce::var(bypassButton.getToggleState()));
	values.add(juce::var(gainSlider.getValue()));
	values.add(juce::var(mixSlider.getValue()));
	values.add(juce::var(volumeSlider.getValue()));

	return values;
}

void AnalogClipperModuleGUI::resized()
{
	auto clipperArea = getContentRenderArea();

	// bypass
	auto temp = clipperArea;
	auto bypassButtonArea = temp.removeFromTop(25);

	bypassButtonArea.setWidth(35.f);
	bypassButtonArea.setX(145.f);
	bypassButtonArea.setY(20.f);

	bypassButton.setBounds(bypassButtonArea);

	auto titleAndBypassArea = clipperArea.removeFromTop(30);
	titleAndBypassArea.translate(0, 4);

	auto clipperGraphArea = clipperArea.removeFromLeft(clipperArea.getWidth() * (4.f / 10.f));
	clipperGraphArea.reduce(10, 10);

	clipperArea.translate(0, 8);

	auto topArea = clipperArea.removeFromTop(clipperArea.getHeight() * (1.f / 2.f));

	auto topLabelsArea = topArea.removeFromTop(14);
	
	// label areas
	temp = topLabelsArea.removeFromLeft(topLabelsArea.getWidth() * (1.f / 2.f));
	auto gainLabelArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
	auto mixLabelArea = temp;
	auto volumeLabelArea = topLabelsArea.removeFromRight(topLabelsArea.getWidth() * (1.f / 2.f));

	// slider areas
	temp = topArea.removeFromLeft(topArea.getWidth() * (1.f / 2.f));
	auto gainArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
	auto mixArea = temp;
	auto volumeArea = topArea.removeFromRight(topArea.getWidth() * (1.f / 2.f));

	juce::Rectangle<int> renderArea;
	renderArea.setSize(gainArea.getWidth(), gainArea.getWidth());
	
	title.setBounds(titleAndBypassArea);
	title.setJustificationType(juce::Justification::centredBottom);

	transferFunctionGraph.setBounds(clipperGraphArea);

	renderArea.setCentre(gainArea.getCentre());
	renderArea.setY(gainArea.getTopLeft().getY());
	gainSlider.setBounds(renderArea);
	gainLabel.setBounds(gainLabelArea);
	gainLabel.setJustificationType(juce::Justification::centred);

	renderArea.setCentre(mixArea.getCentre());
	renderArea.setY(mixArea.getTopLeft().getY());
	mixSlider.setBounds(renderArea);
	mixLabel.setBounds(mixLabelArea);
	mixLabel.setJustificationType(juce::Justification::centred);
	
	renderArea.setCentre(volumeArea.getCentre());
	renderArea.setY(volumeArea.getTopLeft().getY());
	volumeSlider.setBounds(renderArea);
	volumeLabel.setBounds(volumeLabelArea);
	volumeLabel.setJustificationType(juce::Justification::centred);
}
