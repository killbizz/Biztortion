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
	one.setFont(juce::Font("Courier New", 22, 0));
	one.setText("1)", juce::dontSendNotification);

	firstRow.setText("This software is an open-source modular distortion plugin, click on the \"About\" button for more info about author's works", juce::dontSendNotification);
	firstRow.setFont(juce::Font("Courier New", 18, 0));


	two.setFont(juce::Font("Courier New", 22, 0));
	two.setText("2)", juce::dontSendNotification);

	secondRow.setText("Use the slots below to link modules in series and create your own custom processing chain. You can also drag-and-drop the modules to change their position", juce::dontSendNotification);
	secondRow.setFont(juce::Font("Courier New", 18, 0));

	three.setFont(juce::Font("Courier New", 22, 0));
	three.setText("3)", juce::dontSendNotification);

	thirdRow.setText("If you need help with some module features go over the parameter in order to see a tooltip", juce::dontSendNotification);
	thirdRow.setFont(juce::Font("Courier New", 18, 0));

	for (auto* comp : getComps())
	{
		addAndMakeVisible(comp);
	}
}

void HelpComponent::resized()
{
	auto helpArea = getBounds();

	auto numbersArea = helpArea.removeFromLeft(helpArea.getWidth() * (1.f / 10.f));

	auto oneArea = numbersArea.removeFromTop(numbersArea.getHeight() * (1.f / 3.f));
	auto twoArea = numbersArea.removeFromTop(numbersArea.getHeight() * (1.f / 2.f));
	auto threeArea = numbersArea;

	auto firstRowArea = helpArea.removeFromTop(helpArea.getHeight() * (1.f / 3.f));
	auto secondRowArea = helpArea.removeFromTop(helpArea.getHeight() * (1.f / 2.f));
	auto thirdRowArea = helpArea;

	one.setBounds(oneArea);
	one.setJustificationType(juce::Justification::centred);

	two.setBounds(twoArea);
	two.setJustificationType(juce::Justification::centred);

	three.setBounds(threeArea);
	three.setJustificationType(juce::Justification::centred);

	firstRow.setBounds(firstRowArea);
	firstRow.setJustificationType(juce::Justification::centredLeft);

	secondRow.setBounds(secondRowArea);
	secondRow.setJustificationType(juce::Justification::centredLeft);

	thirdRow.setBounds(thirdRowArea);
	thirdRow.setJustificationType(juce::Justification::centredLeft);
}

std::vector<juce::Component*> HelpComponent::getComps()
{
	return {
		&one,
		&two,
		&three,
		&firstRow,
		&secondRow,
		&thirdRow
	};
}
