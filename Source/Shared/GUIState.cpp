/*
  ==============================================================================

    GUIState.cpp

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

#include "GUIState.h"
#include "../Module/ChainModule/ChainModule.h"

GUIState::GUIState(juce::AudioProcessorEditor& ape, PluginState& ps)
    : editor(ape), pluginState(ps)
{
    // 1 - 8 = chain positions with visible components in the chain part of the UI
    for (int i = 0; i < 8; ++i) {
        ChainModuleGUI* item = new ChainModuleGUI(pluginState, *this, i + 1);
        chainModules.push_back(std::unique_ptr<ChainModuleGUI>(item));
    }

    currentGUIModule = std::unique_ptr<GUIModule>(new WelcomeModuleGUI());
    editor.addAndMakeVisible(*currentGUIModule);
    // WARNING: for juce::Component default settings the wantsKeyboardFocus is false
    currentGUIModule->setWantsKeyboardFocus(true);

    inputMeter = std::unique_ptr<GUIModule>(new MeterModuleGUI(pluginState, "Input", pluginState.getMeterSource("Input")));
    editor.addAndMakeVisible(*inputMeter);
    // WARNING: for juce::Component default settings the wantsKeyboardFocus is false
    inputMeter->setWantsKeyboardFocus(true);

    outputMeter = std::unique_ptr<GUIModule>(new MeterModuleGUI(pluginState, "Output", pluginState.getMeterSource("Output")));
    editor.addAndMakeVisible(*outputMeter);
    // WARNING: for juce::Component default settings the wantsKeyboardFocus is false
    outputMeter->setWantsKeyboardFocus(true);

    setupChainModules();

    for (auto it = chainModules.rbegin(); it < chainModules.rend(); ++it)
    {
        editor.addAndMakeVisible(**it);
    }

    // NOTE: restoreLastViewedModule() must NOT be called here. GUIState is the editor's first
    // member, so the editor's own components (title, header buttons) do not exist yet, and
    // restoring triggers editor.resized(). The editor calls it once it is fully constructed.
}

void GUIState::restoreLastViewedModule()
{
    const int chainPosition = pluginState.lastViewedChainPosition.getValue();

    if (chainPosition < 1 || chainPosition > static_cast<int>(chainModules.size()))
        return;

    auto& chainModule = *chainModules[chainPosition - 1];

    if (chainModule.getModuleType() == ModuleType::Uninstantiated)
        return;

    chainModule.openInMainView();
}

void GUIState::setLastViewedChainPosition(int chainPosition)
{
    pluginState.lastViewedChainPosition.setValue(chainPosition);
}

void GUIState::showWelcomeModule()
{
    updateCurrentGUIModule(new WelcomeModuleGUI());
    setLastViewedChainPosition(0);
}

void GUIState::setupChainModules()
{
    auto it = ++ pluginState.DSPmodules.cbegin();
    auto end = --pluginState.DSPmodules.cend();
    while (it < end) {
        auto newModuleIt = chainModules.begin() + (**it).getChainPosition() - 1;
        (**newModuleIt).setup((**it).getModuleType());
        ++it;
    }
}

void GUIState::updateCurrentGUIModule(GUIModule* module)
{
    updateChainModules();
    currentGUIModule = std::unique_ptr<GUIModule>(module);
    editor.addAndMakeVisible(*currentGUIModule);
    // WARNING: for juce::Component default settings the wantsKeyboardFocus is false
    currentGUIModule->setWantsKeyboardFocus(true);
    editor.resized();
}

void GUIState::updateChainModules()
{
    for (auto it = chainModules.begin(); it < chainModules.end(); ++it) {
        (**it).currentModuleActivator.setToggleState(false, juce::NotificationType::dontSendNotification);
        (**it).deleteModule.setToggleState(false, juce::NotificationType::dontSendNotification);
        (**it).dragIcon->setVisible(false);
        (**it).chainPositionLabel.setVisible(true);
        // the slot outline depends on the selection, and toggling a child button only repaints
        // that child's area, so the whole slot has to be refreshed explicitly
        (**it).repaint();
    }
}
