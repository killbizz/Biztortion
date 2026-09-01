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
	: HelpComponent(
		"Build a custom distortion chain by adding processors to the slots below. "
		"Signal flows from left to right; select a slot to edit it and drag slots to reorder them. "
		"Hover a control or use a module's ? button whenever you need a quick explanation.")
{
}

HelpComponent::HelpComponent(const juce::String& descriptionText)
{
	description.setText(descriptionText, juce::dontSendNotification);
	applyStyle();

	for (auto* comp : getComps())
	{
		addAndMakeVisible(comp);
	}
}

void HelpComponent::applyStyle()
{
	description.setFont(juce::Font("Segoe UI", 16.0f, juce::Font::plain));
	description.setColour(juce::Label::textColourId, juce::Colour(238, 240, 242));
	description.setJustificationType(juce::Justification::topLeft);
	description.setMinimumHorizontalScale(1.0f);
}

void HelpComponent::setDescription(const juce::String& descriptionText)
{
	description.setText(descriptionText, juce::dontSendNotification);
	resized();
}

int HelpComponent::getPreferredHeightForWidth(int width) const
{
	const auto textWidth = juce::jmax(1, width - horizontalMargin * 2);

	juce::AttributedString attributed;
	attributed.setWordWrap(juce::AttributedString::byWord);
	attributed.append(description.getText(), description.getFont(),
		description.findColour(juce::Label::textColourId));

	juce::TextLayout layout;
	layout.createLayout(attributed, static_cast<float>(textWidth));

	// juce::Label derives its line count from textArea.getHeight() / font.getHeight() and
	// elides the tail with "..." as soon as that underestimates, so add a line of slack
	const auto slack = description.getFont().getHeight() * 1.5f;

	return juce::roundToInt(layout.getHeight() + slack) + verticalMargin * 2;
}

void HelpComponent::resized()
{
	description.setBounds(getLocalBounds().reduced(horizontalMargin, verticalMargin));
}

std::vector<juce::Component*> HelpComponent::getComps()
{
	return {
		&description
	};
}
