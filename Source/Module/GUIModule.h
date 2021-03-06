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
    virtual ~GUIModule() = default;
    void drawContainer(juce::Graphics& g);
    juce::Rectangle<int> getContentRenderArea();
    void handleParamCompsEnablement(bool bypass);

    // facility for addAndMakeVisible function
    virtual std::vector<juce::Component*> getAllComps() = 0;
    // facility for handleParamCompsEnablement function (enablement or disablement of parameters based on bypass param)
    virtual std::vector<juce::Component*> getParamComps() = 0;
    // update the parameters of this GUIModule using the parameter values from values array
    virtual void updateParameters(const juce::Array<juce::var>& values) = 0;
    // reset the parameters of this GUIModule to default values
    virtual void resetParameters(unsigned int chainPosition) = 0;
    // get parameter values of this GUIModule (vars in the array can be bool or double values)
    virtual juce::Array<juce::var> getParamValues() = 0;

private:
    juce::Rectangle<int> getContainerArea();
};