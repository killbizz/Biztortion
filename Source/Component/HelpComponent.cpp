/*
  ==============================================================================

	HelpComponent.cpp

	Copyright (c) 2021 KillBizz - Gabriel Bizzo

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

#include "HelpComponent.h"

HelpComponent::HelpComponent()
{
	description.setText(
		"Build a custom distortion chain by adding processors to the slots below. "
		"Signal flows from left to right; select a slot to edit it and drag slots to reorder them. "
		"Hover a control or use a module's ? button whenever you need a quick explanation.",
		juce::dontSendNotification);
	description.setFont(juce::Font("Segoe UI", 16.0f, juce::Font::plain));
	description.setColour(juce::Label::textColourId, juce::Colour(238, 240, 242));
	description.setJustificationType(juce::Justification::centredLeft);

	for (auto* comp : getComps())
	{
		addAndMakeVisible(comp);
	}
}

void HelpComponent::resized()
{
	description.setBounds(getLocalBounds().reduced(16, 10));
}

std::vector<juce::Component*> HelpComponent::getComps()
{
	return {
		&description
	};
}
