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
    chainPositionLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    chainPositionLabel.setColour(juce::Label::textColourId, BiztortionTheme::textMuted);
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
    auto deleteIcon = juce::Drawable::createFromImageData(BinaryData::trash_svg, BinaryData::trash_svgSize);
    deleteIcon->replaceColour(juce::Colours::black, BiztortionTheme::textMuted);
    deleteModule.setImages(deleteIcon.get());

    setupDeleteModuleColours(deleteModuleLookAndFeel);
    deleteModule.setLookAndFeel(&deleteModuleLookAndFeel);

    addAndMakeVisible(bypassButton);
    bypassButton.setVisible(false);
    bypassButton.setLookAndFeel(&bypassLookAndFeel);
    bypassButton.setTooltip("Bypass this module");

    // currentModuleActivator
    addAndMakeVisible(currentModuleActivator);
    currentModuleActivator.setClickingTogglesState(true);
    currentModuleActivator.setToggleState(false, juce::dontSendNotification);
    currentModuleActivator.setTooltip("Access this module");

    setupCurrentModuleActivatorColours(currentModuleActivatorLookAndFeel);
    currentModuleActivator.setLookAndFeel(&currentModuleActivatorLookAndFeel);

    // lambdas

    newModule.onClick = [this] {
        if (newModule.getToggleState())
            showNewModuleMenu();
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
    dragIcon->replaceColour(juce::Colours::black, juce::Colours::white);
    addAndMakeVisible(*dragIcon);
    dragIcon->setVisible(false);
    dragIcon->setAlwaysOnTop(true);
}

ChainModuleGUI::~ChainModuleGUI()
{
    newModule.setLookAndFeel(nullptr);
    deleteModule.setLookAndFeel(nullptr);
    bypassButton.setLookAndFeel(nullptr);
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

    auto statusArea = bounds.removeFromTop(22);

    chainPositionLabel.setBounds(statusArea);
    chainPositionLabel.setJustificationType(juce::Justification::centred);

    if (moduleType != ModuleType::Uninstantiated)
    {
        auto dragArea = statusArea.withSizeKeepingCentre(22, 14);
        dragIcon->setTransformToFit(dragArea.toFloat(), juce::RectanglePlacement::centred);
    }

    auto actionArea = bounds.removeFromBottom(32).withSizeKeepingCentre(70, 28);
    auto moduleArea = bounds.reduced(6, 5);
    auto moduleButtonArea = juce::Rectangle<int>(moduleArea.getWidth(), 44)
        .withCentre(moduleArea.getCentre());

    if (moduleType == ModuleType::Uninstantiated)
    {
        newModule.setBounds(juce::Rectangle<int>(42, 36).withCentre(moduleButtonArea.getCentre()));
    }
    else
    {
        currentModuleActivator.setBounds(moduleButtonArea);
        bypassButton.setBounds(actionArea.removeFromLeft(28));
        actionArea.removeFromLeft(14);
        deleteModule.setBounds(actionArea.removeFromLeft(28));
    }

    // Audio Cables 
    if (chainPosition != 8) {
        rightCable->setTransform(getCableTransform());
    }
}

void ChainModuleGUI::timerCallback() {}

std::vector<juce::Component*> ChainModuleGUI::getAllComps()
{
    return {
        &newModule
    };
}

void ChainModuleGUI::showNewModuleMenu()
{
    juce::PopupMenu menu;
    menu.setLookAndFeel(&newModuleSelectorLookAndFeel);
    menu.addItem(ModuleType::Oscilloscope, "Oscilloscope");
    menu.addItem(ModuleType::Equalizer, "Equalizer");
    menu.addSeparator();
    menu.addItem(ModuleType::Waveshaper, "Waveshaper");
    menu.addItem(ModuleType::ClassicBitcrusher, "Classic Bitcrusher");
    menu.addItem(ModuleType::SpectrumBitcrusher, "Spectrum Bitcrusher");
    menu.addItem(ModuleType::SlewLimiter, "Slew Limiter");

    auto safePtr = juce::Component::SafePointer<ChainModuleGUI>(this);
    menu.showMenuAsync(juce::PopupMenu::Options()
                           .withTargetComponent(&newModule)
                           .withMinimumWidth(250)
                           .withStandardItemHeight(34),
        [safePtr](int result) {
            if (auto* comp = safePtr.getComponent())
            {
                comp->newModule.setToggleState(false, juce::dontSendNotification);
                if (result > 0)
                    comp->addNewModule(static_cast<ModuleType>(result));
            }
        });
}

std::vector<juce::Component*> ChainModuleGUI::getParamComps()
{
    return std::vector<juce::Component*>();
}

void ChainModuleGUI::setupNewModuleColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, BiztortionTheme::surfaceRaised);
    laf.setColour(juce::TextButton::textColourOffId, BiztortionTheme::text);

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void ChainModuleGUI::setupNewModuleSelectorColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::PopupMenu::backgroundColourId, BiztortionTheme::surfaceRaised);
    laf.setColour(juce::PopupMenu::textColourId, BiztortionTheme::text);
    laf.setColour(juce::PopupMenu::highlightedBackgroundColourId, BiztortionTheme::controlAccent.withAlpha(0.22f));
    laf.setColour(juce::PopupMenu::highlightedTextColourId, BiztortionTheme::text);
}

void ChainModuleGUI::setupDeleteModuleColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    laf.setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentBlack);
}

void ChainModuleGUI::setupCurrentModuleActivatorColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, BiztortionTheme::surfaceRaised);
    laf.setColour(juce::TextButton::textColourOffId, BiztortionTheme::text);

    laf.setColour(juce::TextButton::buttonOnColourId, moduleType_colors.at(moduleType));
    laf.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
}

void ChainModuleGUI::setup(const ModuleType type)
{
    //chainPositionLabel - newModule - deleteModule - dragIcon - currentModuleActivator setup
    moduleType = type;
    if (type == ModuleType::Uninstantiated) {
        bypassButtonAttachment.reset();
        chainPositionLabel.setVisible(true);
        newModule.setVisible(true);
        deleteModule.setVisible(false);
        bypassButton.setVisible(false);

        dragIcon->setVisible(false);
        currentModuleActivator.setVisible(false);
    }
    else {
        chainPositionLabel.setVisible(false);
        newModule.setVisible(false);
        deleteModule.setVisible(true);
        bypassButton.setVisible(true);

        dragIcon->setVisible(true);
        juce::String typeString = moduleType_names.at(type);
        currentModuleActivator.setVisible(true);
        currentModuleActivator.setButtonText(typeString);
        bypassLookAndFeel.color = moduleType_colors.at(type);
        bypassButtonAttachment = std::make_unique<ButtonAttachment>(
            pluginState.apvts, getBypassParameterID(), bypassButton);
    }
    newModule.setToggleState(false, juce::NotificationType::dontSendNotification);

    setupCurrentModuleActivatorColours(currentModuleActivatorLookAndFeel);
    setupDeleteModuleColours(deleteModuleLookAndFeel);

    resized();
}

juce::String ChainModuleGUI::getBypassParameterID() const
{
    const auto number = std::to_string(
        pluginState.getParameterNumberFromDSPmodule(moduleType, chainPosition));

    switch (moduleType)
    {
        case ModuleType::Oscilloscope:       return "Oscilloscope Bypassed " + number;
        case ModuleType::Equalizer:          return "Filter Bypassed " + number;
        case ModuleType::Waveshaper:         return "Waveshaper Bypassed " + number;
        case ModuleType::ClassicBitcrusher:  return CLASSIC_BITCRUSHER_ID + "Bypassed " + number;
        case ModuleType::SpectrumBitcrusher: return SPECTRUM_BITCRUSHER_ID + "Bypassed " + number;
        case ModuleType::SlewLimiter:         return "SlewLimiter Bypassed " + number;
        default:                              return {};
    }
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
    this->deleteModule.setToggleState(false, juce::NotificationType::dontSendNotification);
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
    /*return juce::AffineTransform::scale(0.07, 0.08).translated((39.f + 112.1f*(chainPosition-1)), 427.f);*/
    /*return juce::AffineTransform::scale(
        JUCE_LIVE_CONSTANT(0.2f), 
        JUCE_LIVE_CONSTANT(0.2f))
        .translated(
            (JUCE_LIVE_CONSTANT(68.8f) + 112.1f * (chainPosition - 1)),
            JUCE_LIVE_CONSTANT(451.86f));*/
    const auto bounds = getBounds();
    return juce::AffineTransform::scale(0.2f, 0.2f)
        .translated(bounds.getX() + bounds.getWidth() * 0.61f + 8.0f,
                    bounds.getY() + 19.0f);
}

std::unique_ptr<Drawable> ChainModuleGUI::getRightCable(unsigned int chainPosition)
{
    switch (chainPosition) {

        case 1: return juce::Drawable::createFromImageData(BinaryData::magentaCable_svg , BinaryData::magentaCable_svgSize);
        case 2: return juce::Drawable::createFromImageData(BinaryData::cyanCable_svg , BinaryData::cyanCable_svgSize);
        case 3: return juce::Drawable::createFromImageData(BinaryData::yellowCable_svg, BinaryData::yellowCable_svgSize);
        case 4: return juce::Drawable::createFromImageData(BinaryData::violetCable_svg, BinaryData::violetCable_svgSize);
        case 5: return juce::Drawable::createFromImageData(BinaryData::orangeCable_svg, BinaryData::orangeCable_svgSize);
        case 6: return juce::Drawable::createFromImageData(BinaryData::blueCable_svg, BinaryData::blueCable_svgSize);
        case 7: return juce::Drawable::createFromImageData(BinaryData::redCable_svg, BinaryData::redCable_svgSize);

        default: return nullptr;
    }
    
}

void ChainModuleGUI::drawContainer(juce::Graphics& g)
{
    auto container = getContainerArea().toFloat();
    g.setColour(BiztortionTheme::surface);
    g.fillRoundedRectangle(container, 7.0f);
    g.setColour(somethingIsBeingDraggedOver ? BiztortionTheme::controlAccent : BiztortionTheme::border);
    g.drawRoundedRectangle(container, 7.0f, somethingIsBeingDraggedOver ? 2.0f : 1.0f);

    if (moduleType != ModuleType::Uninstantiated)
    {
        auto accent = container.removeFromTop(3.0f).reduced(12.0f, 0.0f);
        g.setColour(moduleType_colors.at(moduleType));
        g.fillRoundedRectangle(accent, 1.5f);
    }
}
