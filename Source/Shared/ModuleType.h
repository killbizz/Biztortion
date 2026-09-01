/*
  ==============================================================================

    ModuleType.h

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

// WARNING: the enum values are serialized in the plugin state (see PluginState::addDSPmoduleToAPVTS),
// so new entries must always be appended at the end to keep older sessions loadable.
enum ModuleType {
    Uninstantiated,
    Meter,
    Oscilloscope,
    Equalizer,
    Waveshaper,
    ClassicBitcrusher,
    SpectrumBitcrusher,
    SlewLimiter,
    SpectrumAnalyzer
};

const std::unordered_map<ModuleType, juce::String> moduleType_names({
    {ModuleType::Meter, "Meter"},
    {ModuleType::Oscilloscope, "Scope"},
    {ModuleType::Equalizer, "Equalizer"},
    {ModuleType::Waveshaper, "Waveshaper"},
    {ModuleType::ClassicBitcrusher, "Classic Bitcrusher"},
    {ModuleType::SpectrumBitcrusher, "Spectrum Bitcrusher"},
    {ModuleType::SlewLimiter, "Slew Limiter"},
    {ModuleType::SpectrumAnalyzer, "Analyzer"}
    });

const std::unordered_map<ModuleType, juce::Colour> moduleType_colors({
    {ModuleType::Uninstantiated, juce::Colours::black},
    {ModuleType::Oscilloscope, juce::Colours::magenta},
    {ModuleType::Equalizer, juce::Colours::cyan},
    {ModuleType::Waveshaper, juce::Colours::red},
    {ModuleType::ClassicBitcrusher, juce::Colours::orange},
    {ModuleType::SpectrumBitcrusher, juce::Colour(0,108,255)}, // "medium" blue
    {ModuleType::SlewLimiter, juce::Colour(240,255,0)}, // light yellow
    {ModuleType::SpectrumAnalyzer, juce::Colour(0,230,118)} // spring green
    });

// modules which analyse the spectrum own a pair of FFT analyzer FIFOs (see PluginState).
// keep every FIFO book-keeping site in sync by asking this instead of comparing to a single type.
inline bool moduleTypeOwnsAnalyzerFIFO(ModuleType type)
{
    return type == ModuleType::Equalizer || type == ModuleType::SpectrumAnalyzer;
}