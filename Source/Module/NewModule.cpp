/*
  ==============================================================================

    NewModuleModule.cpp

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

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

#include "NewModule.h"
#include "../PluginProcessor.h"
#include "../PluginEditor.h"
#include <algorithm>

NewModuleGUI::NewModuleGUI(BiztortionAudioProcessor& p, BiztortionAudioProcessorEditor& e, unsigned int _chainPosition)
    : GUIModule(), audioProcessor(p), editor(e), chainPosition(_chainPosition)
{

    // POSSIBILITA'
    // 1. NewModule per istanziare un nuovo modulo => pulsante "+",
    //                                                elenco triggerabile per aggiungere un nuovo modulo
    // 2. NewModule con un modulo già istanziato => pulsante "x" per eliminare modulo dalla catena, 
    //                                              pulsante col nome per triggerare la visualizzazione a schermo della UI del modulo

    chainPositionLabel.setText(juce::String(chainPosition), juce::dontSendNotification);
    chainPositionLabel.setFont(juce::Font("Courier New", 12, 0));
    addAndMakeVisible(chainPositionLabel);

    // newModule
    addAndMakeVisible(newModule);
    newModule.setClickingTogglesState(true);
    newModule.setToggleState(false, juce::dontSendNotification);
    newModule.setTooltip("Create a new module");

    setupNewModuleColours(newModuleLookAndFeel);
    newModule.setLookAndFeel(&newModuleLookAndFeel);

    // deleteModule
    addAndMakeVisible(deleteModule);
    deleteModule.setClickingTogglesState(true);
    deleteModule.setToggleState(true, juce::dontSendNotification);
    deleteModule.setTooltip("Delete the current module");

    setupDeleteModuleColours(deleteModuleLookAndFeel);
    deleteModule.setLookAndFeel(&deleteModuleLookAndFeel);

    // currentModuleActivator
    addAndMakeVisible(currentModuleActivator);
    currentModuleActivator.setTooltip("Access the current module");

    setupCurrentModuleActivatorColours(currentModuleActivatorLookAndFeel);
    currentModuleActivator.setLookAndFeel(&currentModuleActivatorLookAndFeel);

    // newModuleSelector
    newModuleSelector.setJustificationType(juce::Justification::centred);
    // 999 id for the default text entry
    newModuleSelector.addItem("Select one module here", 999);
    newModuleSelector.addItem("Oscilloscope", ModuleType::Oscilloscope);
    newModuleSelector.addItem("Filter", ModuleType::IIRFilter);
    newModuleSelector.addSeparator();
    newModuleSelector.addItem("Waveshaper", ModuleType::Waveshaper);
    newModuleSelector.addItem("Bitcrusher", ModuleType::Bitcrusher);
    newModuleSelector.addItem("Slew Limiter", ModuleType::SlewLimiter);

    newModuleSelector.setSelectedId(999);
    
    setupNewModuleSelectorColours(newModuleSelectorLookAndFeel);
    newModuleSelector.setLookAndFeel(&newModuleSelectorLookAndFeel);

    newModuleSelector.getRootMenu()->setLookAndFeel(&newModuleSelectorLookAndFeel);

    // lambdas

    newModule.onClick = [this] {
        editor.removeChildComponent(&newModuleSelector);
        editor.addAndMakeVisible(newModuleSelector);
        newModuleSelector.setVisible(newModule.getToggleState());
        newModuleSelector.grabKeyboardFocus();
        juce::Rectangle<int> rect{310, 50};
        rect.setCentre(editor.currentGUIModule->getBounds().getCentre());
        newModuleSelector.setBounds(rect);
        if (newModule.getToggleState()) {
            newModuleSelector.showPopup();
        }
    };

    newModuleSelector.onChange = [this] {
        ModuleType type = static_cast<ModuleType>(newModuleSelector.getSelectedId());

        switch (type) {
        case ModuleType::Oscilloscope: {
            audioProcessor.addAndSetupModuleForDSP(audioProcessor.createDSPModule(type), getChainPosition());
            audioProcessor.addDSPmoduleTypeAndPositionToAPVTS(type, getChainPosition());
            addModuleToGUI(createGUIModule(type));
            newModuleSetup(type);
            break;
        }
        case ModuleType::IIRFilter: {
            audioProcessor.addAndSetupModuleForDSP(audioProcessor.createDSPModule(type), getChainPosition());
            audioProcessor.addDSPmoduleTypeAndPositionToAPVTS(type, getChainPosition());
            addModuleToGUI(createGUIModule(type));
            newModuleSetup(type);
            break;
        }

        case ModuleType::Waveshaper: {
            audioProcessor.addAndSetupModuleForDSP(audioProcessor.createDSPModule(type), getChainPosition());
            audioProcessor.addDSPmoduleTypeAndPositionToAPVTS(type, getChainPosition());
            addModuleToGUI(createGUIModule(type));
            newModuleSetup(type);
            break;
        }
        case ModuleType::Bitcrusher: {
            audioProcessor.addAndSetupModuleForDSP(audioProcessor.createDSPModule(type), getChainPosition());
            audioProcessor.addDSPmoduleTypeAndPositionToAPVTS(type, getChainPosition());
            addModuleToGUI(createGUIModule(type));
            newModuleSetup(type);
            break;
        }
        case ModuleType::SlewLimiter: {
            audioProcessor.addAndSetupModuleForDSP(audioProcessor.createDSPModule(type), getChainPosition());
            audioProcessor.addDSPmoduleTypeAndPositionToAPVTS(type, getChainPosition());
            addModuleToGUI(createGUIModule(type));
            newModuleSetup(type);
            break;
        }
        default: {
            break;
        }
        }
    };

    deleteModule.onClick = [this] {
        // remove GUI module
        moduleType = ModuleType::Uninstantiated;
        editor.currentGUIModule = std::unique_ptr<GUIModule>(new WelcomeModuleGUI());
        editor.addAndMakeVisible(*editor.currentGUIModule);
        // WARNING: for juce::Component default settings the wantsKeyboardFocus is false
        editor.currentGUIModule->setWantsKeyboardFocus(true);
        newModuleSetup(moduleType);
        // remove DSP module
        bool found = false;
        for (auto it = audioProcessor.DSPmodules.begin(); !found && it < audioProcessor.DSPmodules.end(); ++it) {
            if ((**it).getChainPosition() == getChainPosition()) {
                found = true;
                audioProcessor.suspendProcessing(true);
                // remove fft analyzer FIFO associated with **it filter
                auto filter = dynamic_cast<FilterModuleDSP*>(&**it);
                if (filter) {
                    audioProcessor.deleteOldAnalyzerFIFO(getChainPosition());
                }
                it = audioProcessor.DSPmodules.erase(it);
                audioProcessor.suspendProcessing(false);
            }
        }
        audioProcessor.removeDSPmoduleTypeAndPositionFromAPVTS(getChainPosition());
    };

    currentModuleActivator.onClick = [this] {
        addModuleToGUI(createGUIModule(moduleType));
        editor.resized();
    };

    // Audio Cables
    //setPaintingIsUnclipped(true);
    if (chainPosition != 8) {
        rightCable = getRightCable(chainPosition);
        editor.addAndMakeVisible(*rightCable);
        rightCable->setAlwaysOnTop(true);
    }
}

NewModuleGUI::~NewModuleGUI()
{
    newModule.setLookAndFeel(nullptr);
    deleteModule.setLookAndFeel(nullptr);
    currentModuleActivator.setLookAndFeel(nullptr);
}

unsigned int NewModuleGUI::getChainPosition()
{
    return chainPosition;
}

void NewModuleGUI::setChainPosition(unsigned int cp)
{
    chainPosition = cp;
}

void NewModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void NewModuleGUI::resized()
{
    auto bounds = getContentRenderArea();

    juce::Rectangle<int> newModuleBounds, deleteModuleBounds, currentModuleActivatorBounds;
    newModuleBounds = deleteModuleBounds = currentModuleActivatorBounds = bounds;
    newModuleBounds.reduce(20, 40);
    deleteModuleBounds.reduce(30, 44);
    currentModuleActivatorBounds.reduce(2, 40);

    auto chainPositionLabelArea = bounds.removeFromTop(bounds.getHeight() * (1.f / 4.f));

    chainPositionLabel.setBounds(chainPositionLabelArea);
    chainPositionLabel.setJustificationType(juce::Justification::centred);

    if (moduleType == ModuleType::Uninstantiated) {
        newModule.setBounds(newModuleBounds);
        newModule.setCentreRelative(0.5f, 0.5f);
    }
    else {
        deleteModule.setBounds(deleteModuleBounds);
        deleteModule.setCentreRelative(0.5f, 0.75f);
        currentModuleActivator.setBounds(currentModuleActivatorBounds);
        currentModuleActivator.setCentreRelative(0.5f, 0.5f);
    }

    // Audio Cables 
    if (chainPosition != 8) {
        rightCable->setTransform(getTransform());
    }
}

std::vector<juce::Component*> NewModuleGUI::getAllComps()
{
    return {
        &newModule,
        &newModuleSelector
    };
}

std::vector<juce::Component*> NewModuleGUI::getParamComps()
{
    return std::vector<juce::Component*>();
}

void NewModuleGUI::setupNewModuleColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    laf.setColour(juce::TextButton::textColourOffId, juce::Colours::black);

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void NewModuleGUI::setupNewModuleSelectorColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey.withAlpha(0.75f));
    laf.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    laf.setColour(juce::ComboBox::outlineColourId, juce::Colours::white);
    laf.setColour(juce::PopupMenu::backgroundColourId, laf.findColour(juce::ComboBox::backgroundColourId));
}

void NewModuleGUI::setupDeleteModuleColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    laf.setColour(juce::TextButton::textColourOffId, juce::Colours::red);

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void NewModuleGUI::setupCurrentModuleActivatorColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    laf.setColour(juce::TextButton::textColourOffId, juce::Colours::green);

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void NewModuleGUI::newModuleSetup(const ModuleType type)
{
    moduleType = type;
    // newModule and deleteModule setup
    if (type == ModuleType::Uninstantiated) {
        newModule.setVisible(true);
        deleteModule.setVisible(false);
    }
    else {
        newModule.setVisible(false);
        deleteModule.setVisible(true);
    }
    newModuleSelector.setSelectedId(999);
    newModuleSelector.setVisible(false);
    newModule.setToggleState(false, juce::NotificationType::dontSendNotification);

    // currentModuleActivator setup
    if (type != ModuleType::Uninstantiated) {
        juce::String typeString;
        switch (type) {
            case ModuleType::Oscilloscope: {
                typeString = "Oscilloscope";
                break;
            }
            case ModuleType::IIRFilter: {
                typeString = "Filter";
                break;
            }
            case ModuleType::Waveshaper: {
                typeString = "Waveshaper";
                break;
            }
            case ModuleType::Bitcrusher: {
                typeString = "Bitcrusher";
                break;
            }
            case ModuleType::SlewLimiter: {
                typeString = "Slew Limiter";
                break;
            }
            default:
                break;
        }
        currentModuleActivator.setVisible(true);
        currentModuleActivator.setButtonText(typeString);
        currentModuleActivator.changeWidthToFitText(10);
    }
    else {
        currentModuleActivator.setVisible(false);
    }
    resized();
    editor.resized();
}

GUIModule* NewModuleGUI::createGUIModule(ModuleType type)
{
    GUIModule* newModule = nullptr;
    switch (type) {
        case ModuleType::IIRFilter: {
            newModule = new FilterModuleGUI(audioProcessor, getChainPosition());
            break;
        }
        case ModuleType::Oscilloscope: {
            OscilloscopeModuleDSP* oscilloscopeDSPModule = nullptr;
            bool found = false;
            for (auto it = audioProcessor.DSPmodules.cbegin(); !found && it < audioProcessor.DSPmodules.cend(); ++it) {
                if ((**it).getChainPosition() == getChainPosition()) {
                    oscilloscopeDSPModule = dynamic_cast<OscilloscopeModuleDSP*>(&**it);
                    found = true;
                }
            }
            newModule = new OscilloscopeModuleGUI(audioProcessor, oscilloscopeDSPModule->getLeftOscilloscope(), oscilloscopeDSPModule->getRightOscilloscope(), getChainPosition());
            break;
        }
        case ModuleType::Waveshaper: {
            newModule = new WaveshaperModuleGUI(audioProcessor, getChainPosition());
            break;
        }
        case ModuleType::Bitcrusher: {
            newModule = new BitcrusherModuleGUI(audioProcessor, getChainPosition());
            break;
        }
        case ModuleType::SlewLimiter: {
            newModule = new SlewLimiterModuleGUI(audioProcessor, getChainPosition());
            break;
        }
        default :
            break;
    }
    return newModule;
}

void NewModuleGUI::addModuleToGUI(GUIModule* module)
{
    for (auto it = editor.newModules.begin(); it < editor.newModules.end(); ++it) {
        (**it).currentModuleActivator.setToggleState(false, juce::NotificationType::dontSendNotification);
        (**it).deleteModule.setToggleState(false, juce::NotificationType::dontSendNotification);
    }
    editor.currentGUIModule = std::unique_ptr<GUIModule>(module);
    this->currentModuleActivator.setToggleState(true, juce::NotificationType::dontSendNotification);
    this->deleteModule.setToggleState(true, juce::NotificationType::dontSendNotification);
    editor.addAndMakeVisible(*editor.currentGUIModule);
    // WARNING: for juce::Component default settings the wantsKeyboardFocus is false
    editor.currentGUIModule->setWantsKeyboardFocus(true);
}

juce::AffineTransform NewModuleGUI::getTransform()
{
    return juce::AffineTransform::scale(0.07, 0.08).translated((39.f + 112.1f*(chainPosition-1)), 427.f);
}

std::unique_ptr<juce::Drawable> NewModuleGUI::getRightCable(unsigned int chainPosition)
{
    switch (chainPosition) {

        case 1: return juce::Drawable::createFromImageData(BinaryData::AudioCableRosa_png, BinaryData::AudioCableRosa_pngSize);
        case 2: return juce::Drawable::createFromImageData(BinaryData::AudioCableVerdeAcqua_png, BinaryData::AudioCableVerdeAcqua_pngSize);
        case 3: return juce::Drawable::createFromImageData(BinaryData::AudioCableGiallo_png, BinaryData::AudioCableGiallo_pngSize);
        case 4: return juce::Drawable::createFromImageData(BinaryData::AudioCableViola_png, BinaryData::AudioCableViola_pngSize);
        case 5: return juce::Drawable::createFromImageData(BinaryData::AudioCableArancione_png, BinaryData::AudioCableArancione_pngSize);
        case 6: return juce::Drawable::createFromImageData(BinaryData::AudioCableBlu_png, BinaryData::AudioCableBlu_pngSize);
        case 7: return juce::Drawable::createFromImageData(BinaryData::AudioCableRosso_png, BinaryData::AudioCableRosso_pngSize);

        default: break;
    }
    
}