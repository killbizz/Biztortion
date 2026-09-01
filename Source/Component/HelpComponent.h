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

// Shared body of every help dialog: the plugin "About Biztortion" window and each
// module's "?" window use this component so they all render with the same style.
class HelpComponent : public juce::Component
{
public:
	//==============================================================================
	// default text = the plugin overview shown by the "About Biztortion" button
	HelpComponent();
	explicit HelpComponent(const juce::String& descriptionText);

	void setDescription(const juce::String& descriptionText);
	// height needed to show the current text at the given width, for sizing the dialog
	int getPreferredHeightForWidth(int width) const;

	void paint(juce::Graphics& g) override {};
	void resized() override;

	std::vector<juce::Component*> getComps();

	static constexpr int horizontalMargin = 16;
	static constexpr int verticalMargin = 10;

private:
	void applyStyle();

	juce::Label description;

	//==============================================================================
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HelpComponent)
};