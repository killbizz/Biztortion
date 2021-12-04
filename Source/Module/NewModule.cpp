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
    newModuleSelector.addItem("Select the module to create", 999);
    newModuleSelector.addItem("Oscilloscope", ModuleType::Oscilloscope);
    newModuleSelector.addItem("Filter", ModuleType::IIRFilter);
    newModuleSelector.addSeparator();
    newModuleSelector.addItem("Waveshaper", ModuleType::Waveshaper);
    newModuleSelector.addItem("Bitcrusher", ModuleType::Bitcrusher);
    newModuleSelector.addItem("Slew Limiter", ModuleType::SlewLimiter);

    newModuleSelector.setSelectedId(999);
    //newModuleSelector.setTooltip("Select the module to create");
    
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
        if(type != 999)
            addNewModule(type);
    };

    deleteModule.onClick = [this] {
        deleteTheCurrentNewModule();
    };

    currentModuleActivator.onClick = [this] {
        addModuleToGUI(createGUIModule(moduleType, getChainPosition()));
    };

    // Audio Cables
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

ModuleType NewModuleGUI::getModuleType()
{
    return moduleType;
}

void NewModuleGUI::setModuleType(ModuleType mt)
{
    moduleType = mt;
}

void NewModuleGUI::addNewModule(ModuleType type)
{
    audioProcessor.addAndSetupModuleForDSP(audioProcessor.createDSPModule(type), getChainPosition());
    audioProcessor.addDSPmoduleTypeAndPositionToAPVTS(type, getChainPosition());
    addModuleToGUI(createGUIModule(type, getChainPosition()));
    newModuleSetup(type);
}

void NewModuleGUI::deleteTheCurrentNewModule()
{
    // remove GUI module
    moduleType = ModuleType::Uninstantiated;
    // reset the parameter values to default
    editor.currentGUIModule->resetParameters(getChainPosition());
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
    editor.resized();
}

void NewModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
    // draw a red line around the comp if the user's currently dragging something over it..
    if (somethingIsBeingDraggedOver)
    {
        g.setColour(Colours::red);
        g.drawRect(getLocalBounds(), 3);
    }
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
    //editor.resized();
}

// These methods implement the DragAndDropTarget interface, and allow our component
// to accept drag-and-drop of objects from other JUCE components..

bool NewModuleGUI::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // normally you'd check the sourceDescription value to see if it's the
    // sort of object that you're interested in before returning true, but for
    // the demo, we'll say yes to anything..
    auto component = dynamic_cast<NewModuleGUI*>(dragSourceDetails.sourceComponent.get());
    if (component) {
        if ((component->getChainPosition() == getChainPosition()) || (getModuleType() != ModuleType::Uninstantiated)) {
            return false;
        }
        else {
            return true;
        }
    }
    else {
        return false;
    }
}

void NewModuleGUI::itemDragEnter(const SourceDetails& /*dragSourceDetails*/)
{
    somethingIsBeingDraggedOver = true;
    repaint();
}

void NewModuleGUI::itemDragMove(const SourceDetails& /*dragSourceDetails*/)
{
}

void NewModuleGUI::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
    somethingIsBeingDraggedOver = false;
    repaint();
}

void NewModuleGUI::itemDropped(const SourceDetails& dragSourceDetails)
{
    auto component = dynamic_cast<NewModuleGUI*>(dragSourceDetails.sourceComponent.get());
    // add NewModule to this chain position
    //addNewModule(component->getModuleType());
    auto type = component->getModuleType();
    audioProcessor.addAndSetupModuleForDSP(audioProcessor.createDSPModule(type), getChainPosition());
    audioProcessor.addDSPmoduleTypeAndPositionToAPVTS(type, getChainPosition());
    newModuleSetup(type);
    // copy all the parameter values and reset the old ones
    GUIModule* oldModule = createGUIModule(component->getModuleType(), component->getChainPosition());
    GUIModule* newModule = createGUIModule(getModuleType(), getChainPosition());
    newModule->updateParameters(oldModule);
    // delete the old GUImodule from the previous chain position and reset it's parameters
    component->deleteTheCurrentNewModule();
    // add the new GUImodule to the editor
    addModuleToGUI(newModule);
    somethingIsBeingDraggedOver = false;
    repaint();
}

GUIModule* NewModuleGUI::createGUIModule(ModuleType type, unsigned int chainPosition)
{
    GUIModule* newModule = nullptr;
    switch (type) {
        case ModuleType::IIRFilter: {
            newModule = new FilterModuleGUI(audioProcessor, chainPosition);
            break;
        }
        case ModuleType::Oscilloscope: {
            OscilloscopeModuleDSP* oscilloscopeDSPModule = nullptr;
            bool found = false;
            for (auto it = audioProcessor.DSPmodules.cbegin(); !found && it < audioProcessor.DSPmodules.cend(); ++it) {
                if ((**it).getChainPosition() == chainPosition) {
                    oscilloscopeDSPModule = dynamic_cast<OscilloscopeModuleDSP*>(&**it);
                    found = true;
                }
            }
            newModule = new OscilloscopeModuleGUI(audioProcessor, oscilloscopeDSPModule->getLeftOscilloscope(), oscilloscopeDSPModule->getRightOscilloscope(), chainPosition);
            break;
        }
        case ModuleType::Waveshaper: {
            newModule = new WaveshaperModuleGUI(audioProcessor, chainPosition);
            break;
        }
        case ModuleType::Bitcrusher: {
            newModule = new BitcrusherModuleGUI(audioProcessor, chainPosition);
            break;
        }
        case ModuleType::SlewLimiter: {
            newModule = new SlewLimiterModuleGUI(audioProcessor, chainPosition);
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
    editor.resized();
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

void DragTextButton::mouseDrag(const MouseEvent& event)
{
    auto newModule = dynamic_cast<NewModuleGUI*>(getParentComponent());
    auto editor = dynamic_cast<BiztortionAudioProcessorEditor*>(getParentComponent()->getParentComponent());
    // array { chainPosition, moduleType }
    /*juce::var sourceDescription = juce::var(juce::Array<juce::var>());
    sourceDescription.append((int) newModule->getChainPosition());
    sourceDescription.append((int) newModule->getModuleType());*/
    editor->startDragging("", newModule);
}
