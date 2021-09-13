/*
  ==============================================================================

    WelcomeModule.h
    Created: 13 Sep 2021 2:59:45pm
    Author:  gabri

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "GUIModule.h"

// This is basically a sawtooth wave generator - maps a value that bounces between
// 0.0 and 1.0 at a random speed
struct BouncingNumber
{
    BouncingNumber()
        : speed(0.0004 + 0.0007 * juce::Random::getSystemRandom().nextDouble()),
        phase(juce::Random::getSystemRandom().nextDouble())
    {
    }

    float getValue() const
    {
        double v = fmod(phase + speed * juce::Time::getMillisecondCounterHiRes(), 2.0);
        return (float)(v >= 1.0 ? (2.0 - v) : v);
    }

protected:
    double speed, phase;
};

struct SlowerBouncingNumber : public BouncingNumber
{
    SlowerBouncingNumber()
    {
        speed *= 0.3;
    }
};

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
};