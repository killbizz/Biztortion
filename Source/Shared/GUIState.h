/*
  ==============================================================================

    GUIState.h

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
#include "../Module/GUIModule.h"
#include "../Module/ChainModule.h"

class GUIState {
public:

    GUIState(juce::AudioProcessorEditor& ape, PluginState& ps);

    // quick way to access the audio processor
    juce::AudioProcessorEditor& editor;

    // --------- GUI ---------
    std::unique_ptr<GUIModule> currentGUIModule;
    std::vector<std::unique_ptr<ChainModuleGUI>> chainModules;
    std::unique_ptr<GUIModule> inputMeter;
    std::unique_ptr<GUIModule> outputMeter;

    void setupChainModules();
    void updateCurrentGUIModule(GUIModule* module);

private:

    PluginState& pluginState;

};