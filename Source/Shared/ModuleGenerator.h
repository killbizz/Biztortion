/*
  ==============================================================================

    ModuleGenerator.h

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

#include "../Shared/PluginState.h"
#include "../Shared/ModuleType.h"

#include "../Module/DSPModule.h"
#include "../Module/GUIModule.h"
#include "../Module/ClassicBitcrusherModule.h"
#include "../Module/SpectrumBitcrusherModule.h"
#include "../Module/WaveshaperModule.h"
#include "../Module/FilterModule.h"
#include "../Module/MeterModule.h"
#include "../Module/OscilloscopeModule.h"
#include "../Module/SlewLimiterModule.h"
#include "../Module/AnalogClipperModule/AnalogClipperModule.h"

class ModuleGenerator {
public:

    // SOFTWARE EXTENSIBILITY : guide to add a new chain module:
    // - create 2 new classes which extend DSPModule and GUIModule
    // - add a new entry in the ModuleType enum and in the moduleType_names map (in ../Shared/ModuleType.h file)
    // - update the createDSPModule and createGUIModule methods
    // - add the addParameters function of the newly created DSPModule in the ../Shared/PluginState::createParameterLayout()

    ModuleGenerator(PluginState& s);

    DSPModule* createDSPModule(ModuleType mt);
    GUIModule* createGUIModule(ModuleType type, unsigned int parameterNumber);

private:

    PluginState& pluginState;

};
