/*
  ==============================================================================

    GUIModule.h

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

class GUIModule : public juce::Component {
public:
    void drawContainer(juce::Graphics& g);
    virtual std::vector<juce::Component*> getComps() = 0;
    juce::Rectangle<int> getContentRenderArea();
private:
    juce::Rectangle<int> getContainerArea();
};