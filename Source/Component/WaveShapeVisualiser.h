/*
  ==============================================================================

    WaveShapeVisualiser.h

    Copyright (c) 2022 KillBizz - Gabriel Bizzo

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

#include <functional>

/**
    Shows what a module is doing to the signal: a reference sine cycle is pushed through a
    caller supplied shaping function and the result is drawn over it.

    The shaper is a simplified, purely visual model of the module's DSP (it is evaluated on the
    message thread, never in the audio callback), so it only has to be faithful enough to make
    the effect of each parameter obvious.
*/
class WaveShapeVisualiser : public juce::Component,
                            public juce::SettableTooltipClient,
                            private juce::Timer
{
public:
    enum class Style
    {
        waveform,          // smooth curve
        steppedWaveform,   // staircase, for modules that quantise the signal
        spectrumBars       // magnitude bars, for modules that work on the spectrum
    };

    /** Processes the buffer in place.
        For the waveform styles it holds one reference cycle in -1..1; for spectrumBars it holds
        bar magnitudes in 0..1.
    */
    using Shaper = std::function<void(std::vector<float>&)>;

    WaveShapeVisualiser(juce::Colour accentColour, Shaper shaperToUse,
                        Style styleToUse = Style::waveform);

    void setAccentColour(juce::Colour newColour);
    void setStyle(Style newStyle);
    // short label drawn in a corner, e.g. "output"
    void setCaption(juce::String newCaption);

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;

    void paintWaveform(juce::Graphics& g, juce::Rectangle<float> plot);
    void paintSpectrumBars(juce::Graphics& g, juce::Rectangle<float> plot);

    juce::Path buildPath(const std::vector<float>& samples, juce::Rectangle<float> area,
                         bool stepped) const;

    juce::Colour accent;
    Shaper shaper;
    juce::String caption;
    Style style = Style::waveform;

    // number of points of the reference cycle: enough for a smooth curve, cheap to redraw
    static constexpr int resolution = 256;
    // whole number of cycles, so the reference has zero mean and the DC filter only shifts the
    // wave when the other shared parameters have actually made it asymmetric
    static constexpr float cycles = 2.0f;
    // number of magnitude bars in spectrumBars style
    static constexpr int barCount = 28;
};
