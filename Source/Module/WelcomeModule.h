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

// FOR SVG ANIMATION
// 
// This is basically a sawtooth wave generator - maps a value that bounces between
// 0.0 and 1.0 at a random speed
//struct BouncingNumber
//{
//    BouncingNumber()
//        : speed(0.0004 + 0.0007 * juce::Random::getSystemRandom().nextDouble()),
//        phase(juce::Random::getSystemRandom().nextDouble())
//    {
//    }
//
//    float getValue() const
//    {
//        double v = fmod(phase + speed * juce::Time::getMillisecondCounterHiRes(), 2.0);
//        return (float)(v >= 1.0 ? (2.0 - v) : v);
//    }
//
//protected:
//    double speed, phase;
//};
//
//struct SlowerBouncingNumber : public BouncingNumber
//{
//    SlowerBouncingNumber()
//    {
//        speed *= 0.3;
//    }
//};

//==============================================================================

/* WelcomeModule GUI */

//==============================================================================

class WelcomeModuleGUI : public GUIModule {
public:
    WelcomeModuleGUI();

    void paint(juce::Graphics& g) override;
    void resized() override;
    std::vector<juce::Component*> getComps() override;

    juce::AffineTransform getTransform();

private:

    std::unique_ptr<juce::Drawable> bigNose;
    // data fields for svg animation
    // SlowerBouncingNumber position, rotation;
    juce::Label title, 
        one, firstRow,
        two, secondRow,
        three, thirdRow;
};