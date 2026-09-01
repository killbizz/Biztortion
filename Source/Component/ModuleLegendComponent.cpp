/*
  ==============================================================================

    ModuleLegendComponent.cpp

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

#include "ModuleLegendComponent.h"

#include "../Shared/GUIStuff.h"

ModuleLegendComponent::ModuleLegendComponent()
{
    // listed in the same order as the "create a new module" menu
    for (const auto type : { ModuleType::Oscilloscope, ModuleType::SpectrumAnalyzer,
                             ModuleType::Equalizer, ModuleType::Waveshaper,
                             ModuleType::ClassicBitcrusher, ModuleType::SpectrumBitcrusher,
                             ModuleType::SlewLimiter })
    {
        entries.push_back({ moduleType_colors.at(type), moduleType_names.at(type) });
    }

    const juce::Font nameFont("Segoe UI", 14.0f, juce::Font::plain);
    auto widest = 0;
    for (const auto& entry : entries)
        widest = juce::jmax(widest, juce::GlyphArrangement::getStringWidthInt(nameFont, entry.name));

    setSize(widest + margin * 2 + 26,
            titleHeight + static_cast<int>(entries.size()) * rowHeight + margin);
}

void ModuleLegendComponent::paint(juce::Graphics& g)
{
    g.fillAll(BiztortionTheme::surface);

    auto bounds = getLocalBounds().reduced(margin, 0);

    auto titleArea = bounds.removeFromTop(titleHeight);
    g.setColour(BiztortionTheme::text);
    g.setFont(juce::Font("Segoe UI", 15.0f, juce::Font::bold));
    g.drawText("Module Legend", titleArea, juce::Justification::centredLeft, false);

    g.setFont(juce::Font("Segoe UI", 14.0f, juce::Font::plain));

    for (const auto& entry : entries)
    {
        auto row = bounds.removeFromTop(rowHeight);

        const auto dot = row.removeFromLeft(14).withSizeKeepingCentre(10, 10).toFloat();
        g.setColour(entry.colour);
        g.fillEllipse(dot);

        row.removeFromLeft(12);
        g.setColour(BiztortionTheme::textMuted);
        g.drawText(entry.name, row, juce::Justification::centredLeft, true);
    }
}
