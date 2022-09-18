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

enum ModuleType {
    Uninstantiated,
    Meter,
    Oscilloscope,
    IIRFilter,
    Waveshaper,
    ClassicBitcrusher,
    SpectrumBitcrusher,
    SlewLimiter,
	AnalogClipper
};

const std::unordered_map<ModuleType, juce::String> moduleType_names({
    {ModuleType::Meter, "Meter"},
    {ModuleType::Oscilloscope, "Scope"},
    {ModuleType::IIRFilter, "Filter"},
    {ModuleType::Waveshaper, "Waveshaper"},
    {ModuleType::ClassicBitcrusher, "Classic Bitcrusher"},
    {ModuleType::SpectrumBitcrusher, "Spectrum Bitcrusher"},
	{ModuleType::SlewLimiter, "Slew Limiter"},
	{ModuleType::AnalogClipper, "Analog Clipper"}
    });
