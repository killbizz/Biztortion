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

//==============================================================================

CableComponent::CableComponent()
{
    // purely decorative: never steal clicks from the chain slots underneath
    setInterceptsMouseClicks(false, false);
}

void CableComponent::setAnimationPhase(float newPhase)
{
    newPhase = juce::jlimit(0.0f, 1.0f, newPhase);

    if (! juce::approximatelyEqual(phase, newPhase))
    {
        phase = newPhase;
        repaint();
    }
}

void CableComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    if (bounds.isEmpty())
        return;

    const auto scaleX = bounds.getWidth() / artworkWidth;
    const auto scaleY = bounds.getHeight() / artworkHeight;
    // stroke widths cannot be scaled per-axis, so use the smaller factor for them
    const auto strokeScale = juce::jmin(scaleX, scaleY);
    const auto toComponent = juce::AffineTransform::scale(scaleX, scaleY);

    const juce::Point<float> leftPlug(48.2f, 48.2f);
    const juce::Point<float> rightPlug(382.65f, 49.25f);

    juce::Path cable;
    cable.startNewSubPath(leftPlug);
    cable.cubicTo({ 48.2f, 146.0f }, { 382.65f, 146.0f }, rightPlug);
    cable.applyTransform(toComponent);

    const juce::PathStrokeType bodyStroke(46.0f * strokeScale,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path bodyShape;
    bodyStroke.createStrokedPath(bodyShape, cable);

    // --- cable body: white, brightening and dimming along its length as a sine that travels
    // left to right, so the eye follows the audio down the chain. Filling the stroked shape
    // with a horizontal gradient keeps it smooth: drawing it as separate segments would band
    // where their round caps overlap.
    const auto brightnessAt = [this](float proportion) {
        const auto wave = 0.5f + 0.5f * std::sin(juce::MathConstants<float>::twoPi
                                                 * (proportion - phase));
        return juce::Colours::white.withAlpha(0.22f + 0.78f * wave);
    };

    constexpr int gradientStops = 24;
    juce::ColourGradient flow(brightnessAt(0.0f), bounds.getX(), 0.0f,
                              brightnessAt(1.0f), bounds.getRight(), 0.0f, false);

    for (int stop = 1; stop < gradientStops; ++stop)
    {
        const auto proportion = (float) stop / (float) gradientStops;
        flow.addColour(proportion, brightnessAt(proportion));
    }

    g.setGradientFill(flow);
    g.fillPath(bodyShape);

    // --- outline: a hairline that stays solid white while the body dims ---
    g.setColour(juce::Colours::white);
    g.strokePath(bodyShape, juce::PathStrokeType(0.5f));

    // --- plugs: stable white rounded ends ---
    const auto plugRadius = 38.5f * strokeScale;
    const auto plugRingWidth = 7.5f * strokeScale;
    const auto plugCoreRadius = 15.0f * strokeScale;

    for (const auto& plug : { leftPlug, rightPlug })
    {
        const auto centre = plug.transformedBy(toComponent);
        const auto ring = juce::Rectangle<float>(plugRadius * 2.0f, plugRadius * 2.0f)
                              .withCentre(centre);

        g.setColour(BiztortionTheme::canvas);
        g.fillEllipse(ring);
        g.setColour(juce::Colours::white);
        g.drawEllipse(ring.reduced(plugRingWidth * 0.5f), plugRingWidth);

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.fillEllipse(juce::Rectangle<float>(plugCoreRadius * 2.0f, plugCoreRadius * 2.0f)
                          .withCentre(centre));
    }
}

//==============================================================================

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
    currentModuleActivator.setWantsKeyboardFocus(true);
    currentModuleActivator.onRenameRequested = [this] { beginRenaming(); };

    // rename editor, shown on top of the slot button only while renaming
    addChildComponent(renameLabel);
    renameLabel.setEditable(true);
    renameLabel.setJustificationType(juce::Justification::centred);
    renameLabel.setFont(ModuleLookAndFeel::getLabelsFont());
    renameLabel.setColour(juce::Label::backgroundColourId, BiztortionTheme::surfaceRaised);
    renameLabel.setColour(juce::Label::textColourId, BiztortionTheme::text);
    renameLabel.setColour(juce::Label::backgroundWhenEditingColourId, BiztortionTheme::surfaceRaised);
    renameLabel.setColour(juce::Label::textWhenEditingColourId, BiztortionTheme::text);
    renameLabel.onEditorHide = [this] { commitRename(); };

    // juce::Label's editor keeps its own colours and justification, so style it when it appears
    renameLabel.onEditorShow = [this] {
        if (auto* editor = renameLabel.getCurrentTextEditor())
        {
            const auto accent = moduleType == ModuleType::Uninstantiated
                ? BiztortionTheme::controlAccent
                : moduleType_colors.at(moduleType);

            editor->setJustification(juce::Justification::centred);
            editor->setFont(ModuleLookAndFeel::getLabelsFont());
            editor->setColour(juce::TextEditor::backgroundColourId, BiztortionTheme::surfaceRaised);
            editor->setColour(juce::TextEditor::textColourId, BiztortionTheme::text);
            editor->setColour(juce::TextEditor::outlineColourId, accent);
            editor->setColour(juce::TextEditor::focusedOutlineColourId, accent);
            editor->setColour(juce::TextEditor::highlightColourId, accent.withAlpha(0.35f));
            editor->setColour(juce::TextEditor::highlightedTextColourId, BiztortionTheme::canvas);
            editor->setSelectAllWhenFocused(true);
        }
    };

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
        openInMainView();
    };

    // Audio Cables
    if (chainPosition != 8) {
        rightCable = std::make_unique<CableComponent>();
        guiState.editor.addAndMakeVisible(*rightCable);
        rightCable->setAlwaysOnTop(true);
        startTimerHz(30);
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
    // a deleted module must not leave its custom name behind for the next module reusing
    // the same type/parameter number
    pluginState.clearModuleCustomName(moduleType, thisParamNumber);
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
        guiState.showWelcomeModule();
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
    // draw a white line around the comp if the user's currently dragging something over it..
    if (somethingIsBeingDraggedOver)
    {
        g.setColour(Colours::white);
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
        rightCable->setBounds(getCableBounds());
    }
}

void ChainModuleGUI::timerCallback()
{
    if (rightCable == nullptr)
        return;

    // 0.36 rev/s: half the previous 0.72 speed. The offset per chain position keeps the
    // pulse travelling along the chain instead of every cable blinking in unison.
    const auto seconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
    auto phase = seconds * 0.36 - static_cast<double>(chainPosition - 1) * 0.13;
    phase -= std::floor(phase);

    rightCable->setAnimationPhase(static_cast<float>(phase));
}

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

    // each entry carries the module's chain colour, so the menu also teaches the colour coding
    const auto addModuleItem = [&menu](ModuleType type, const juce::String& name) {
        menu.addCustomItem(type,
            std::make_unique<ModuleMenuItemComponent>(moduleType_colors.at(type), name),
            nullptr, name);
    };

    menu.addSectionHeader("Analysis");
    addModuleItem(ModuleType::Oscilloscope, "Oscilloscope");
    addModuleItem(ModuleType::SpectrumAnalyzer, "Spectrum Analyzer");
    menu.addSeparator();
    menu.addSectionHeader("Tone");
    addModuleItem(ModuleType::Equalizer, "Equalizer");
    menu.addSeparator();
    menu.addSectionHeader("Distortion");
    addModuleItem(ModuleType::Waveshaper, "Waveshaper");
    addModuleItem(ModuleType::ClassicBitcrusher, "Classic Bitcrusher");
    addModuleItem(ModuleType::SpectrumBitcrusher, "Spectrum Bitcrusher");
    addModuleItem(ModuleType::SlewLimiter, "Slew Limiter");

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
    currentModuleActivator.setColour(BiztortionTheme::accentOutlineColourId,
                                     moduleType_colors.at(moduleType));
}

void ChainModuleGUI::setup(const ModuleType type)
{
    //chainPositionLabel - newModule - deleteModule - dragIcon - currentModuleActivator setup
    moduleType = type;
    if (type == ModuleType::Uninstantiated) {
        bypassButtonAttachment.reset();
        renameLabel.setVisible(false);
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
        currentModuleActivator.setVisible(true);
        updateModuleNameDisplay();
        bypassLookAndFeel.color = moduleType_colors.at(type);
        bypassButtonAttachment = std::make_unique<ButtonAttachment>(
            pluginState.apvts, getBypassParameterID(), bypassButton);
    }
    newModule.setToggleState(false, juce::NotificationType::dontSendNotification);

    setupCurrentModuleActivatorColours(currentModuleActivatorLookAndFeel);
    setupDeleteModuleColours(deleteModuleLookAndFeel);

    resized();
    repaint();
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
        case ModuleType::SpectrumAnalyzer:    return "SpectrumAnalyzer Bypassed " + number;
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


unsigned int ChainModuleGUI::getParameterNumber() const
{
    return pluginState.getParameterNumberFromDSPmodule(moduleType, chainPosition);
}

// Shows the module name in the slot button, and keeps the tooltip in sync with it.
void ChainModuleGUI::updateModuleNameDisplay()
{
    if (moduleType == ModuleType::Uninstantiated)
        return;

    const auto typeName = moduleType_names.at(moduleType);
    const auto displayName = pluginState.getModuleDisplayName(moduleType, getParameterNumber());

    currentModuleActivator.setButtonText(displayName);
    currentModuleActivator.setTooltip("Access the " + displayName + " (" + typeName + ") module");
}

void ChainModuleGUI::beginRenaming()
{
    if (moduleType == ModuleType::Uninstantiated)
        return;

    renameLabel.setBounds(currentModuleActivator.getBounds());
    renameLabel.setText(pluginState.getModuleDisplayName(moduleType, getParameterNumber()),
                        juce::dontSendNotification);
    renameLabel.setVisible(true);
    renameLabel.toFront(true);
    renameLabel.showEditor();
}

void ChainModuleGUI::commitRename()
{
    renameLabel.setVisible(false);

    if (moduleType == ModuleType::Uninstantiated)
        return;

    const auto newName = renameLabel.getText().trim();
    const auto parameterNumber = getParameterNumber();

    // an empty name, or the module type name itself, means "no custom name"
    if (newName.isEmpty() || newName == moduleType_names.at(moduleType))
        pluginState.clearModuleCustomName(moduleType, parameterNumber);
    else
        pluginState.setModuleCustomName(moduleType, parameterNumber, newName);

    updateModuleNameDisplay();
    repaint();
}

void ChainModuleGUI::openInMainView()
{
    if (moduleType == ModuleType::Uninstantiated)
        return;

    addModuleToGUI(moduleGenerator.createGUIModule(
        moduleType, pluginState.getParameterNumberFromDSPmodule(moduleType, getChainPosition())));
    dragIcon->setVisible(true);
    chainPositionLabel.setVisible(false);
}

void ChainModuleGUI::addModuleToGUI(GUIModule* module)
{
    guiState.updateCurrentGUIModule(module);
    // remember this slot so reopening the editor comes back to it
    guiState.setLastViewedChainPosition(static_cast<int>(getChainPosition()));
    this->currentModuleActivator.setToggleState(true, juce::NotificationType::dontSendNotification);
    this->deleteModule.setToggleState(false, juce::NotificationType::dontSendNotification);
    // this slot just became the selected one: repaint it whole so its outline is complete
    repaint();
    for (auto it = guiState.chainModules.begin(); it < guiState.chainModules.end(); ++it) {
        auto& chainModule = **it;
        if (chainModule.getChainPosition() != getChainPosition()) {
            chainModule.dragIcon->setVisible(false);
            chainModule.chainPositionLabel.setVisible(true);
        }
    }
}

// The cable bridges this slot and the next one, in the editor's coordinate space.
// Sized from the artwork's aspect ratio at the scale the SVG used to be drawn at (0.2).
juce::Rectangle<int> ChainModuleGUI::getCableBounds() const
{
    const auto bounds = getBounds();
    constexpr auto scale = 0.2f;

    return juce::Rectangle<int>(
        bounds.getX() + juce::roundToInt(bounds.getWidth() * 0.61f) + 8,
        bounds.getY() + 19,
        juce::roundToInt(431.65f * scale),
        juce::roundToInt(169.68f * scale));
}

void ChainModuleGUI::drawContainer(juce::Graphics& g)
{
    auto container = getContainerArea().toFloat();
    g.setColour(BiztortionTheme::surface);
    g.fillRoundedRectangle(container, 7.0f);

    // The slot whose module is currently open in the main view wears its module colour as a
    // full outline; every other slot keeps the neutral border so only one slot stands out.
    const auto isCurrentlyShown = moduleType != ModuleType::Uninstantiated
        && currentModuleActivator.getToggleState();
    g.setColour(isCurrentlyShown ? moduleType_colors.at(moduleType) : BiztortionTheme::border);
    g.drawRoundedRectangle(container, 7.0f,
                           somethingIsBeingDraggedOver || isCurrentlyShown ? 2.0f : 1.0f);

    if (moduleType != ModuleType::Uninstantiated)
    {
        auto accent = container.removeFromTop(3.0f).reduced(12.0f, 0.0f);
        g.setColour(moduleType_colors.at(moduleType));
        g.fillRoundedRectangle(accent, 1.5f);
    }
}
