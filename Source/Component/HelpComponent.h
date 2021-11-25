/*
  ==============================================================================

	HelpComponent.h

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

#pragma once

#include <JuceHeader.h>

class HelpComponent : public juce::Component
{
public:
	//==============================================================================
	HelpComponent();

	void paint(juce::Graphics& g) override {};
	void resized() override;

	std::vector<juce::Component*> getComps();

private:

	juce::Label one, firstRow,
		two, secondRow,
		three, thirdRow;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpComponent)
};