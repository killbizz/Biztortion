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

enum ModuleType {
    Uninstantiated,
    Meter,
    IIRFilter,
    Oscilloscope,
    Waveshaper,
    Bitcrusher,
    SlewLimiter
};

#pragma once

#define MODULE_TYPES \
X(Uninstantiated, "Uninstantiated") \
X(Meter, "Meter") \
X(IIRFilter, "Filter") \
X(Oscilloscope, "Oscilloscope") \
X(Waveshaper, "Waveshaper") \
X(Bitcrusher, "Bitcrusher") \
X(SlewLimiter, "SlewLimiter")

#define X(type, typeName) type,
enum ModuleType : size_t
{
    MODULE_TYPES
};
#undef X

#define X(type, typeName) typeName,
char const* moduleType_name[] =
{
    MODULE_TYPES
};
#undef X