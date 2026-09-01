/*
  ==============================================================================

    SharedParamsLayout.h

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

#include <array>
#include <vector>

/**
    Layout helper for the six parameters every distortion module shares
    (drive, mix, fx distribution, fx bias, symmetry, dc filter).

    They are arranged on the vertices of a hexagon with a graph in the middle showing their
    combined effect on the signal, which keeps them visually grouped and clearly separate from
    the module's own parameters.
*/
namespace SharedParamsLayout
{
    // vertices, clockwise from the top
    inline constexpr int numberOfSlots = 6;

    struct Slot
    {
        // where the knob (or toggle) goes
        juce::Rectangle<int> control;
        // caption strip directly above the control
        juce::Rectangle<int> label;
    };

    struct Hexagon
    {
        std::array<Slot, numberOfSlots> slots;
        // the middle of the hexagon, for the shared parameters graph
        juce::Rectangle<int> centre;
    };

    /**
        Builds the hexagon inside the given area.

        @param area          the region the six parameters may use
        @param labelHeight   height of each caption strip
    */
    inline Hexagon build(juce::Rectangle<int> area, int labelHeight = 14)
    {
        Hexagon hexagon;

        // A regular hexagon with vertices on the vertical axis is far taller than wide and does
        // not fit a module panel, so it is flattened: two cells on the top row, two on the
        // bottom row, and one centred on each side. Everything scales with the given area.
        const auto centre = area.getCentre();

        // size of one parameter cell (caption + control)
        const auto cellWidth = juce::jlimit(56, 100, area.getWidth() / 3);
        const auto cellHeight = juce::jlimit(52, 92, area.getHeight() / 3);
        const auto controlSize = juce::jmin(cellWidth, cellHeight - labelHeight);

        // side cells sit against the left and right edges, the top/bottom pairs are pulled in
        const auto sideRadius = juce::jmax(cellWidth / 2, (area.getWidth() - cellWidth) / 2);
        const auto cornerRadius = juce::roundToInt(sideRadius * 0.56f);
        const auto rowRadius = juce::jmax(cellHeight / 2, (area.getHeight() - cellHeight) / 2);

        // offsets of the six vertices, clockwise from the top-left one
        const std::array<juce::Point<int>, numberOfSlots> offsets {
            juce::Point<int>(-cornerRadius, -rowRadius),   // top left
            juce::Point<int>( cornerRadius, -rowRadius),   // top right
            juce::Point<int>( sideRadius,    0),           // right
            juce::Point<int>( cornerRadius,  rowRadius),   // bottom right
            juce::Point<int>(-cornerRadius,  rowRadius),   // bottom left
            juce::Point<int>(-sideRadius,    0)            // left
        };

        for (int index = 0; index < numberOfSlots; ++index)
        {
            auto cell = juce::Rectangle<int>(cellWidth, cellHeight)
                            .withCentre(centre + offsets[(size_t) index])
                            // keep every cell inside the available area
                            .constrainedWithin(area);

            Slot slot;
            slot.label = cell.removeFromTop(labelHeight);
            slot.control = juce::Rectangle<int>(controlSize, controlSize).withCentre(cell.getCentre());
            hexagon.slots[(size_t) index] = slot;
        }

        // the middle is whatever is left once the side cells and the two rows are reserved
        hexagon.centre = juce::Rectangle<int>(
                juce::jmax(80, area.getWidth() - cellWidth * 2 - 28),
                juce::jmax(60, area.getHeight() - cellHeight * 2 - 20))
            .withCentre(centre)
            .constrainedWithin(area);

        return hexagon;
    }

    /**
        Purely visual model of what the shared parameters do to the signal, used by the graph in
        the middle of the hexagon. Mirrors the order of operations of DSPModule
        (symmetry -> drive -> effect distribution around the bias -> dry/wet mix).

        @param samples          reference cycle, processed in place
        @param driveDb          0..40 dB
        @param mixPercent       0..100
        @param fxDistribution   -100..100
        @param bias             -0.9..0.9
        @param symmetryPercent  -100..100
        @param removeDCoffset   whether the DC filter is enabled
    */
    inline void applySharedParameters(std::vector<float>& samples, float driveDb, float mixPercent,
                                      float fxDistribution, float bias, float symmetryPercent,
                                      bool removeDCoffset)
    {
        if (samples.empty())
            return;

        const auto driveGain = juce::Decibels::decibelsToGain(driveDb);
        const auto wetMix = juce::jlimit(0.0f, 1.0f, mixPercent * 0.01f);
        const auto distribution = juce::jlimit(-1.0f, 1.0f, fxDistribution * 0.01f);
        // DSPModule::applySymmetry works with 0..2, where 1 leaves the signal untouched
        const auto symmetryAmount = juce::jmap(symmetryPercent, -100.0f, 100.0f, 0.0f, 2.0f);

        // Follows the order the modules process in: drive -> effect -> effect distribution ->
        // symmetry -> dc filter, and only then the dry/wet mix. The module's own effect is
        // stood in for by the hard clipper the modules end with, so with drive at 0 dB the wet
        // path equals the dry one and none of these parameters changes the picture - which is
        // exactly what happens to the audio.
        std::vector<float> processed(samples.size());
        auto mean = 0.0f;

        for (size_t index = 0; index < samples.size(); ++index)
        {
            const auto driven = samples[index] * driveGain;
            const auto effected = juce::jlimit(-1.0f, 1.0f, driven);

            // DSPModule::effectDistribution: full effect on one side of the bias threshold,
            // blended with the unaffected signal on the other side
            auto value = effected;

            if (! juce::approximatelyEqual(distribution, 0.0f))
            {
                const auto keptDryGain = std::abs(distribution);
                const auto fullyEffected = distribution > 0.0f ? driven >= bias : driven < bias;

                if (! fullyEffected)
                    value = driven * keptDryGain + effected * (1.0f - keptDryGain);
            }

            // DSPModule::applySymmetry
            value *= value < 0.0f ? (2.0f - symmetryAmount) : symmetryAmount;

            processed[index] = value;
            mean += value;
        }

        // the dc filter runs on the wet path, before it is mixed back with the dry signal
        if (removeDCoffset)
        {
            mean /= (float) processed.size();

            for (auto& value : processed)
                value -= mean;
        }

        for (size_t index = 0; index < samples.size(); ++index)
            samples[index] = juce::jlimit(-1.0f,
                                          1.0f,
                                          samples[index] * (1.0f - wetMix) + processed[index] * wetMix);
    }
}
