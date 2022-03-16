/*
  ==============================================================================

    ChainModule.cpp

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

#include "ChainModule.h"

#include <algorithm>

ChainModuleGUI::ChainModuleGUI(PluginState& ps, GUIState& gs, unsigned int _chainPosition)
    : GUIModule(), pluginState(ps), guiState(gs), chainPosition(_chainPosition), moduleFactory(pluginState)
{
    chainPositionLabel.setText(juce::String(chainPosition), juce::dontSendNotification);
    chainPositionLabel.setFont(ModuleLookAndFeel::getLabelsFont());
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
    newModuleSelector.addSeparator();
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
        guiState.editor.removeChildComponent(&newModuleSelector);
        guiState.editor.addAndMakeVisible(newModuleSelector);
        newModuleSelector.setVisible(newModule.getToggleState());
        newModuleSelector.grabKeyboardFocus();
        juce::Rectangle<int> rect{310, 50};
        rect.setCentre(guiState.currentGUIModule->getBounds().getCentre());
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
        addModuleToGUI(moduleFactory.createGUIModule(moduleType, getChainPosition()));
    };

    // Audio Cables
    if (chainPosition != 8) {
        rightCable = getRightCable(chainPosition);
        guiState.editor.addAndMakeVisible(*rightCable);
        rightCable->setAlwaysOnTop(true);
    }
}

ChainModuleGUI::~ChainModuleGUI()
{
    newModule.setLookAndFeel(nullptr);
    deleteModule.setLookAndFeel(nullptr);
    currentModuleActivator.setLookAndFeel(nullptr);
}

unsigned int ChainModuleGUI::getChainPosition()
{
    return chainPosition;
}

void ChainModuleGUI::setChainPosition(unsigned int cp)
{
    chainPosition = cp;
}

ModuleType ChainModuleGUI::getModuleType()
{
    return moduleType;
}

void ChainModuleGUI::setModuleType(ModuleType mt)
{
    moduleType = mt;
}

void ChainModuleGUI::addNewModule(ModuleType type)
{
    pluginState.addAndSetupModuleForDSP(moduleFactory.createDSPModule(type), getChainPosition());
    pluginState.addDSPmoduleToAPVTS(type, getChainPosition());
    addModuleToGUI(moduleFactory.createGUIModule(type, getChainPosition()));
    this->setup(type);
}

void ChainModuleGUI::deleteTheCurrentNewModule()
{
    // remove GUI module
    moduleType = ModuleType::Uninstantiated;
    // reset the parameter values to default
    guiState.currentGUIModule->resetParameters(getChainPosition());
    guiState.updateCurrentGUIModule(new WelcomeModuleGUI());
    this->setup(moduleType);
    // remove DSP module
    pluginState.removeModuleFromDSPmodules(getChainPosition());
    pluginState.removeDSPmoduleFromAPVTS(getChainPosition());
}

void ChainModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
    // draw a red line around the comp if the user's currently dragging something over it..
    if (somethingIsBeingDraggedOver)
    {
        g.setColour(Colours::red);
        g.drawRect(getLocalBounds(), 3);
    }
}

void ChainModuleGUI::resized()
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

std::vector<juce::Component*> ChainModuleGUI::getAllComps()
{
    return {
        &newModule,
        &newModuleSelector
    };
}

std::vector<juce::Component*> ChainModuleGUI::getParamComps()
{
    return std::vector<juce::Component*>();
}

void ChainModuleGUI::setupNewModuleColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    laf.setColour(juce::TextButton::textColourOffId, juce::Colours::black);

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void ChainModuleGUI::setupNewModuleSelectorColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey.withAlpha(0.75f));
    laf.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    laf.setColour(juce::ComboBox::outlineColourId, juce::Colours::white);
    laf.setColour(juce::PopupMenu::backgroundColourId, laf.findColour(juce::ComboBox::backgroundColourId));
}

void ChainModuleGUI::setupDeleteModuleColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    laf.setColour(juce::TextButton::textColourOffId, juce::Colours::red);

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void ChainModuleGUI::setupCurrentModuleActivatorColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    laf.setColour(juce::TextButton::textColourOffId, juce::Colours::green);

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void ChainModuleGUI::setup(const ModuleType type)
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
        juce::String typeString = moduleType_names.at(type);
        currentModuleActivator.setVisible(true);
        currentModuleActivator.setButtonText(typeString);
        currentModuleActivator.changeWidthToFitText(10);
    }
    else {
        currentModuleActivator.setVisible(false);
    }
    resized();
}

// These methods implement the DragAndDropTarget interface, and allow our component
// to accept drag-and-drop of objects from other JUCE components..

bool ChainModuleGUI::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    // normally you'd check the sourceDescription value to see if it's the
    // sort of object that you're interested in before returning true, but for
    // the demo, we'll say yes to anything..
    auto component = dynamic_cast<ChainModuleGUI*>(dragSourceDetails.sourceComponent.get());
    if (component) {
        if (component->getChainPosition() == getChainPosition()) {
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

void ChainModuleGUI::itemDragEnter(const SourceDetails& /*dragSourceDetails*/)
{
    somethingIsBeingDraggedOver = true;
    repaint();
}

void ChainModuleGUI::itemDragMove(const SourceDetails& /*dragSourceDetails*/)
{
}

void ChainModuleGUI::itemDragExit(const SourceDetails& /*dragSourceDetails*/)
{
    somethingIsBeingDraggedOver = false;
    repaint();
}

// drag-and-drop logic : changes one module position or swap two modules
void ChainModuleGUI::itemDropped(const SourceDetails& dragSourceDetails)
{
    auto component = dynamic_cast<ChainModuleGUI*>(dragSourceDetails.sourceComponent.get());
    auto type = component->getModuleType();
    auto cp = component->getChainPosition();
    auto thisModuleType = moduleType;
    bool oneModuleIsAllocatedHere = getModuleType() != ModuleType::Uninstantiated;
    // add newPosition DSPModule
    pluginState.addAndSetupModuleForDSP(moduleFactory.createDSPModule(type), getChainPosition());
    pluginState.addDSPmoduleToAPVTS(type, getChainPosition());
    if (oneModuleIsAllocatedHere) {
        // add oldPosition DSPModule
        pluginState.addAndSetupModuleForDSP(moduleFactory.createDSPModule(getModuleType()), cp);
        pluginState.addDSPmoduleToAPVTS(getModuleType(), cp);
    }
    // setup NewModuleGUIs
    setup(type);
    if (oneModuleIsAllocatedHere) {
        component->setup(thisModuleType);
    } else {
        component->setup(ModuleType::Uninstantiated);
    }
    // create necessary GUIModules
    GUIModule* preModuleInOldPosition = moduleFactory.createGUIModule(type, cp);
    GUIModule* postModuleInOldPosition = moduleFactory.createGUIModule(thisModuleType, cp);
    GUIModule* preModuleInNewPosition = moduleFactory.createGUIModule(thisModuleType, getChainPosition());
    GUIModule* postModuleInNewPosition = moduleFactory.createGUIModule(type, getChainPosition());
    auto preModuleOldPositionParamValues = preModuleInOldPosition->getParamValues();
    juce::Array<juce::var> preModuleNewPositionParamValues;
    if (oneModuleIsAllocatedHere) {
        preModuleNewPositionParamValues = preModuleInNewPosition->getParamValues();
    }
    // update parameters
    postModuleInNewPosition->updateParameters(preModuleOldPositionParamValues);
    if (oneModuleIsAllocatedHere) {
        postModuleInOldPosition->updateParameters(preModuleNewPositionParamValues);
    }
    // reset the parameters to default
    if (type != thisModuleType) {
        preModuleInOldPosition->resetParameters(cp);
        if (oneModuleIsAllocatedHere) {
            preModuleInNewPosition->resetParameters(getChainPosition());
        }
    }
    // delete GUIModules
    delete preModuleInOldPosition;
    delete postModuleInOldPosition;
    delete preModuleInNewPosition;
    delete postModuleInNewPosition;
    // delete the editor currentGUIModule (is mandatory before deleting the DSP modules for the oscilloscope module)
    delete guiState.currentGUIModule.release();
    if (oneModuleIsAllocatedHere) {
        // delete preNewPositionDSPModule
        pluginState.removeModuleFromDSPmodules(getChainPosition());
        pluginState.removeDSPmoduleFromAPVTS(getChainPosition());
    }
    // delete preOldPositionDSPModule
    pluginState.removeModuleFromDSPmodules(cp);
    pluginState.removeDSPmoduleFromAPVTS(cp);
    // add the fresh GUImodule to the editor (is mandatory to create a new GUIModule after deleting the DSP modules for the filter module)
    addModuleToGUI(moduleFactory.createGUIModule(type, getChainPosition()));
    somethingIsBeingDraggedOver = false;
    repaint();
}

void ChainModuleGUI::mouseDrag(const MouseEvent& event)
{
    auto editor = dynamic_cast<DragAndDropContainer*>(getParentComponent());
    if(getModuleType() != ModuleType::Uninstantiated)
        editor->startDragging("", this);
}


void ChainModuleGUI::addModuleToGUI(GUIModule* module)
{
    guiState.updateCurrentGUIModule(module);
    this->currentModuleActivator.setToggleState(true, juce::NotificationType::dontSendNotification);
    this->deleteModule.setToggleState(true, juce::NotificationType::dontSendNotification);
}

juce::AffineTransform ChainModuleGUI::getTransform()
{
    return juce::AffineTransform::scale(0.07, 0.08).translated((39.f + 112.1f*(chainPosition-1)), 427.f);
}

std::unique_ptr<BizDrawable> ChainModuleGUI::getRightCable(unsigned int chainPosition)
{
    switch (chainPosition) {

    case 1: return std::unique_ptr<BizDrawable>(new BizDrawable(*dynamic_cast<juce::DrawableImage*>(
            &*juce::Drawable::createFromImageData(BinaryData::AudioCableRosa_png, BinaryData::AudioCableRosa_pngSize))));
        case 2: return std::unique_ptr<BizDrawable>(new BizDrawable(*dynamic_cast<juce::DrawableImage*>(
            &*juce::Drawable::createFromImageData(BinaryData::AudioCableVerdeAcqua_png, BinaryData::AudioCableVerdeAcqua_pngSize))));
        case 3: return std::unique_ptr<BizDrawable>(new BizDrawable(*dynamic_cast<juce::DrawableImage*>(
            &*juce::Drawable::createFromImageData(BinaryData::AudioCableGiallo_png, BinaryData::AudioCableGiallo_pngSize))));
        case 4: return std::unique_ptr<BizDrawable>(new BizDrawable(*dynamic_cast<juce::DrawableImage*>(
            &*juce::Drawable::createFromImageData(BinaryData::AudioCableViola_png, BinaryData::AudioCableViola_pngSize))));
        case 5: return std::unique_ptr<BizDrawable>(new BizDrawable(*dynamic_cast<juce::DrawableImage*>(
            &*juce::Drawable::createFromImageData(BinaryData::AudioCableArancione_png, BinaryData::AudioCableArancione_pngSize))));
        case 6: return std::unique_ptr<BizDrawable>(new BizDrawable(*dynamic_cast<juce::DrawableImage*>(
            &*juce::Drawable::createFromImageData(BinaryData::AudioCableBlu_png, BinaryData::AudioCableBlu_pngSize))));
        case 7: return std::unique_ptr<BizDrawable>(new BizDrawable(*dynamic_cast<juce::DrawableImage*>(
            &*juce::Drawable::createFromImageData(BinaryData::AudioCableRosso_png, BinaryData::AudioCableRosso_pngSize))));

        default: break;
    }
    
}