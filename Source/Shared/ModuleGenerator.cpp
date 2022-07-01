/*
  ==============================================================================

    ModuleGenerator.cpp

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

#include "ModuleGenerator.h"

ModuleGenerator::ModuleGenerator(PluginState& s) : pluginState(s)
{
}

DSPModule* ModuleGenerator::createDSPModule(ModuleType mt)
{
    DSPModule* newModule = nullptr;
    switch (mt) {
    case ModuleType::Oscilloscope: {
        newModule = new OscilloscopeModuleDSP(pluginState.apvts);
        break;
    }
    case ModuleType::IIRFilter: {
        newModule = new FilterModuleDSP(pluginState.apvts);
        break;
    }
    case ModuleType::Waveshaper: {
        newModule = new WaveshaperModuleDSP(pluginState.apvts);
        break;
    }
    case ModuleType::Bitcrusher: {
        newModule = new BitcrusherModuleDSP(pluginState.apvts);
        break;
    }
    case ModuleType::SlewLimiter: {
        newModule = new SlewLimiterModuleDSP(pluginState.apvts);
        break;
    }
    default:
        break;
    }
    return newModule;
}

GUIModule* ModuleGenerator::createGUIModule(ModuleType type, unsigned int parameterNumber)
{
    GUIModule* newModule = nullptr;
    switch (type) {
        case ModuleType::IIRFilter: {
            newModule = new FilterModuleGUI(pluginState, parameterNumber);
            break;
        }
        case ModuleType::Oscilloscope: {
            // [A] check : if there are 2 OscilloscopeModuleDSP in the same chainPosition i need the second one (the first is gonna be deleted, used only for changing chain order)
            OscilloscopeModuleDSP* oscilloscopeDSPModule = nullptr;
            bool found = false;
            for (auto it = pluginState.DSPmodules.cbegin(); !found && it < pluginState.DSPmodules.cend(); ++it) {
                auto temp = dynamic_cast<OscilloscopeModuleDSP*>(&**it);
                if ((**it).getChainPosition() == parameterNumber && temp) {
                    auto next = it;
                    ++next;
                    // [A]
                    if (next != pluginState.DSPmodules.cend() && (**next).getChainPosition() == parameterNumber && dynamic_cast<OscilloscopeModuleDSP*>(&**next)) {
                        oscilloscopeDSPModule = dynamic_cast<OscilloscopeModuleDSP*>(&**next);
                    }
                    else {
                        oscilloscopeDSPModule = temp;
                    }
                    found = true;
                }
            }
            newModule = new OscilloscopeModuleGUI(pluginState, oscilloscopeDSPModule->getLeftOscilloscope(), oscilloscopeDSPModule->getRightOscilloscope(), parameterNumber);
            break;
        }
        case ModuleType::Waveshaper: {
            newModule = new WaveshaperModuleGUI(pluginState, parameterNumber);
            break;
        }
        case ModuleType::Bitcrusher: {
            newModule = new BitcrusherModuleGUI(pluginState, parameterNumber);
            break;
        }
        case ModuleType::SlewLimiter: {
            newModule = new SlewLimiterModuleGUI(pluginState, parameterNumber);
            break;
        }
        default:
            break;
    }
    return newModule;
}
