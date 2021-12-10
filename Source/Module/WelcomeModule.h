/*
  ==============================================================================

    WelcomeModule.h

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
#include "GUIModule.h"

//==============================================================================

/* WelcomeModule GUI */

//==============================================================================

class WelcomeModuleGUI : public GUIModule, juce::Timer {
public:
    WelcomeModuleGUI();

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    std::vector<juce::Component*> getAllComps() override;
    std::vector<juce::Component*> getParamComps() override;
    // useless methods because (at this moment) this is not a module usable in the processing chain
    virtual void updateParameters(const juce::Array<juce::var>&) override {};
    virtual void resetParameters(unsigned int) override {};
    virtual juce::Array<juce::var> getParamValues() override { return juce::Array<juce::var>(); };

    juce::AffineTransform getTransform();

private:

    std::unique_ptr<juce::Drawable> bigNose;
    juce::Image bigNoseImg;

    juce::Label author, title, version;

    unsigned int hueCounter = 0;
};