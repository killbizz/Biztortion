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
#include "../Shared/PluginState.h"

class ChainModuleGUI;

class GUIState {
public:

    GUIState(juce::AudioProcessorEditor& ape, PluginState& ps);

    void setupChainModules();
    void updateCurrentGUIModule(GUIModule* module);
    void updateChainModules();

    // Opens the module the user was looking at when the editor was last closed. Falls back to
    // leaving the homepage on screen when nothing was stored (freshly created plugin) or when
    // that chain slot no longer holds a module.
    void restoreLastViewedModule();
    // 1-8 for a chain slot, 0 for the homepage
    void setLastViewedChainPosition(int chainPosition);
    void showWelcomeModule();

    //==============================================================================

    juce::AudioProcessorEditor& editor;
    // --------- GUI ---------
    std::unique_ptr<GUIModule> currentGUIModule;
    std::vector<std::unique_ptr<ChainModuleGUI>> chainModules;
    std::unique_ptr<GUIModule> inputMeter;
    std::unique_ptr<GUIModule> outputMeter;

private:

    PluginState& pluginState;

};