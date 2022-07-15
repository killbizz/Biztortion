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
    : GUIModule(), pluginState(ps), guiState(gs), chainPosition(_chainPosition), moduleGenerator(pluginState)
{
    chainPositionLabel.setText(juce::String(chainPosition), juce::dontSendNotification);
    chainPositionLabel.setFont(Font("Courier New", 14, juce::Font::bold));
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
    deleteModule.setVisible(false);
    deleteModule.setTooltip("Delete this module");
    deleteModule.setImages(&*juce::Drawable::createFromImageData(BinaryData::trash_svg, BinaryData::trash_svgSize));

    setupDeleteModuleColours(deleteModuleLookAndFeel);
    deleteModule.setLookAndFeel(&deleteModuleLookAndFeel);

    // currentModuleActivator
    addAndMakeVisible(currentModuleActivator);
    currentModuleActivator.setClickingTogglesState(true);
    currentModuleActivator.setToggleState(false, juce::dontSendNotification);
    currentModuleActivator.setTooltip("Access this module");

    setupCurrentModuleActivatorColours(currentModuleActivatorLookAndFeel);
    currentModuleActivator.setLookAndFeel(&currentModuleActivatorLookAndFeel);

    // newModuleSelector
    newModuleSelector.setJustificationType(juce::Justification::centred);
    // 999 id for the default text entry
    newModuleSelector.addItem("Select the module to create", 999);
    newModuleSelector.addSeparator();
    newModuleSelector.addItem("Audio Scope", ModuleType::Oscilloscope);
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
        deleteTheCurrentChainModule();
    };

    currentModuleActivator.onClick = [this] {
        addModuleToGUI(moduleGenerator.createGUIModule(moduleType, pluginState.getParameterNumberFromDSPmodule(moduleType, getChainPosition())));
        dragIcon->setVisible(true);
        chainPositionLabel.setVisible(false);
    };

    // Audio Cables
    if (chainPosition != 8) {
        rightCable = getRightCable(chainPosition);
        guiState.editor.addAndMakeVisible(*rightCable);
        rightCable->setAlwaysOnTop(true);
    }

    // Drag-And-Drop Icon
    dragIcon = juce::Drawable::createFromImageData(BinaryData::draghorizontal_svg, BinaryData::draghorizontal_svgSize);
    addAndMakeVisible(*dragIcon);
    dragIcon->setVisible(false);
    dragIcon->setAlwaysOnTop(true);
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
    unsigned int parameterNumber = pluginState.addDSPmoduleToAPVTS(type, getChainPosition());
    pluginState.addAndSetupModuleForDSP(moduleGenerator.createDSPModule(type), type, getChainPosition(), parameterNumber);
    addModuleToGUI(moduleGenerator.createGUIModule(type, parameterNumber));
    this->setup(type);
}

void ChainModuleGUI::deleteTheCurrentChainModule()
{
    auto thisParamNumber = pluginState.getParameterNumberFromDSPmodule(moduleType, getChainPosition());
    // reset the parameter values to default
    GUIModule* guiModule = moduleGenerator.createGUIModule(moduleType, thisParamNumber);
    guiModule->resetParameters(thisParamNumber);
    delete guiModule;
    // setup GUI
    bool atLeastOneChainModuleIsPresent = false;
    for (auto it = guiState.chainModules.begin(); !atLeastOneChainModuleIsPresent && it < guiState.chainModules.end(); ++it) {
        auto& chainModule = **it;
        if (chainModule.getModuleType() != ModuleType::Uninstantiated && chainModule.getChainPosition() != getChainPosition()) {
            atLeastOneChainModuleIsPresent = true;
            auto paramNumber = pluginState.getParameterNumberFromDSPmodule(chainModule.getModuleType(), chainModule.getChainPosition());
            chainModule.addModuleToGUI(moduleGenerator.createGUIModule(chainModule.getModuleType(), paramNumber));
            chainModule.dragIcon->setVisible(true);
            chainModule.chainPositionLabel.setVisible(false);
        }
    }
    if (!atLeastOneChainModuleIsPresent) {
        guiState.updateCurrentGUIModule(new WelcomeModuleGUI());
    }
    // remove DSP module
    pluginState.removeModuleFromDSPmodules(getChainPosition());
    pluginState.removeDSPmoduleFromAPVTS(getChainPosition(), moduleType, thisParamNumber);
    // setup of the chain module
    moduleType = ModuleType::Uninstantiated;
    this->setup(moduleType);
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
    deleteModuleBounds.reduce(30, 46);
    currentModuleActivatorBounds.reduce(2, 38);

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
        // Drag-And-Drop Icon
        dragIcon->setTransform(getDragIconTransform());
    }

    // Audio Cables 
    if (chainPosition != 8) {
        rightCable->setTransform(getCableTransform());
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
    laf.setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::red);
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
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
    //chainPositionLabel - newModule - deleteModule - dragIcon - currentModuleActivator setup
    moduleType = type;
    if (type == ModuleType::Uninstantiated) {
        chainPositionLabel.setVisible(true);
        newModule.setVisible(true);
        deleteModule.setVisible(false);

        dragIcon->setVisible(false);
        currentModuleActivator.setVisible(false);
    }
    else {
        chainPositionLabel.setVisible(false);
        newModule.setVisible(false);
        deleteModule.setVisible(true);

        dragIcon->setVisible(true);
        juce::String typeString = moduleType_names.at(type);
        currentModuleActivator.setVisible(true);
        currentModuleActivator.setButtonText(typeString);
        currentModuleActivator.changeWidthToFitText(10);
    }
    newModuleSelector.setSelectedId(999);
    newModuleSelector.setVisible(false);
    newModule.setToggleState(false, juce::NotificationType::dontSendNotification);

    resized();
}

// These methods implement the DragAndDropTarget interface, and allow our component
// to accept drag-and-drop of objects from other JUCE components..

bool ChainModuleGUI::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
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

// DRAG-AND-DROP LOGIC : changing one module position or swapping between two modules
void ChainModuleGUI::itemDropped(const SourceDetails& dragSourceDetails)
{
    auto component = dynamic_cast<ChainModuleGUI*>(dragSourceDetails.sourceComponent.get());
    auto componentType = component->getModuleType();
    auto componentChainPosition = component->getChainPosition();
    auto componentParamNumber = pluginState.getParameterNumberFromDSPmodule(componentType, componentChainPosition);
    auto thisModuleType = moduleType;
    auto thisChainPosition = chainPosition;
    auto thisParamNumber = pluginState.getParameterNumberFromDSPmodule(thisModuleType, thisChainPosition);
    thisParamNumber = thisParamNumber == 0 ? thisChainPosition : thisParamNumber;

    bool oneModuleIsAllocatedHere = getModuleType() != ModuleType::Uninstantiated;

    // add newPosition DSPModule
    pluginState.addDSPmoduleToAPVTS(componentType, thisChainPosition, componentParamNumber);
    pluginState.addAndSetupModuleForDSP(moduleGenerator.createDSPModule(componentType), componentType, thisChainPosition, componentParamNumber);
    if (oneModuleIsAllocatedHere) {
        // add oldPosition DSPModule
        pluginState.addDSPmoduleToAPVTS(thisModuleType, componentChainPosition, thisParamNumber);
        pluginState.addAndSetupModuleForDSP(moduleGenerator.createDSPModule(thisModuleType), thisModuleType, componentChainPosition, thisParamNumber);
    }
    // setup NewModuleGUIs
    setup(componentType);
    if (oneModuleIsAllocatedHere) {
        component->setup(thisModuleType);
    } else {
        component->setup(ModuleType::Uninstantiated);
    }
    // create necessary GUIModules
    GUIModule* preModuleInOldPosition = moduleGenerator.createGUIModule(componentType, componentParamNumber);
    GUIModule* postModuleInOldPosition = moduleGenerator.createGUIModule(thisModuleType, thisParamNumber);
    GUIModule* preModuleInNewPosition = moduleGenerator.createGUIModule(thisModuleType, thisParamNumber);
    GUIModule* postModuleInNewPosition = moduleGenerator.createGUIModule(componentType, componentParamNumber);
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
    /*if (componentType != thisModuleType) {
        preModuleInOldPosition->resetParameters(componentParamNumber);
        if (oneModuleIsAllocatedHere) {
            preModuleInNewPosition->resetParameters(thisParamNumber);
        }
    }*/
    // delete GUIModules
    delete preModuleInOldPosition;
    delete postModuleInOldPosition;
    delete preModuleInNewPosition;
    delete postModuleInNewPosition;
    // delete the editor.currentGUIModule (is mandatory before deleting the DSP modules for the oscilloscope)
    delete guiState.currentGUIModule.release();
    if (oneModuleIsAllocatedHere) {
        // delete preNewPositionDSPModule
        pluginState.removeModuleFromDSPmodules(thisChainPosition);
        pluginState.removeDSPmoduleFromAPVTS(thisChainPosition, thisModuleType, thisParamNumber);
    }
    // delete preOldPositionDSPModule
    pluginState.removeModuleFromDSPmodules(componentChainPosition);
    pluginState.removeDSPmoduleFromAPVTS(componentChainPosition, componentType, componentParamNumber);
    // add the fresh GUImodule to the editor (is mandatory to create a new GUIModule after deleting the DSP modules for the filter module)
    addModuleToGUI(moduleGenerator.createGUIModule(componentType, componentParamNumber));
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
    for (auto it = guiState.chainModules.begin(); it < guiState.chainModules.end(); ++it) {
        auto& chainModule = **it;
        if (chainModule.getChainPosition() != getChainPosition()) {
            chainModule.dragIcon->setVisible(false);
            chainModule.chainPositionLabel.setVisible(true);
        }
    }
}

juce::AffineTransform ChainModuleGUI::getCableTransform()
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

juce::AffineTransform ChainModuleGUI::getDragIconTransform()
{
    return juce::AffineTransform::scale(0.9f, 0.9f).translated(45.6715f, 18.7671);
}
