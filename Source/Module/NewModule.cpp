/*
  ==============================================================================

    NewModule.cpp
    Created: 2 Sep 2021 10:11:11am
    Author:  gabri

  ==============================================================================
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
    // 3. NewModule per Pre/Post-Filters => visualizzabile solo il nome del filtro, modulo non rimuovibile

    chainPositionLabel.setText(juce::String(chainPosition), juce::dontSendNotification);
    chainPositionLabel.setFont(10);
    addAndMakeVisible(chainPositionLabel);

    // newModule
    addAndMakeVisible(newModule);
    newModule.setClickingTogglesState(true);
    newModule.setToggleState(false, juce::dontSendNotification);

    setupNewModuleColours(newModuleLookAndFeel);
    newModule.setLookAndFeel(&newModuleLookAndFeel);

    // deleteModule
    addAndMakeVisible(deleteModule);

    setupDeleteModuleColours(deleteModuleLookAndFeel);
    deleteModule.setLookAndFeel(&deleteModuleLookAndFeel);

    // currentModuleActivator
    addAndMakeVisible(currentModuleActivator);

    setupCurrentModuleActivatorColours(currentModuleActivatorLookAndFeel);
    currentModuleActivator.setLookAndFeel(&currentModuleActivatorLookAndFeel);

    newModuleSelector.addItem("Select one module here", 999);
    newModuleSelector.addItem("Oscilloscope", 1);
    newModuleSelector.addItem("Filter", 2); // ?
    newModuleSelector.addItem("Waveshaper", 3);
    newModuleSelector.addItem("Bitcrusher", 4);
    newModuleSelector.addItem("Clipper", 5);

    newModuleSelector.setSelectedId(999);

    newModule.onClick = [this] {
        editor.addAndMakeVisible(newModuleSelector);
        newModuleSelector.setVisible(newModule.getToggleState());
        auto newModuleBounds = editor.currentGUIModule->getBounds();
        newModuleBounds.reduce(230.f, 175.f);
        newModuleBounds.setCentre(editor.currentGUIModule->getBounds().getCentre());
        newModuleSelector.setBounds(newModuleBounds);
    };

    newModuleSelector.onChange = [this] {
        
        switch (newModuleSelector.getSelectedId()) {
        case 1: {
            audioProcessor.suspendProcessing(true);
            DSPModule* oscilloscopeDSPModule = new OscilloscopeModuleDSP(audioProcessor.apvts);
            addModuleToDSPmodules(oscilloscopeDSPModule);
            audioProcessor.prepareToPlay(audioProcessor.getSampleRate(), audioProcessor.getNumSamples());
            GUIModule* oscilloscopeGUIModule = new OscilloscopeModuleGUI(audioProcessor, dynamic_cast<OscilloscopeModuleDSP*>(oscilloscopeDSPModule)->getOscilloscope());
            addModuleToGUI(oscilloscopeGUIModule);
            audioProcessor.suspendProcessing(false);
            // NewModule reset and hide
            newModuleSelector.setSelectedId(999);
            newModuleSelector.setVisible(false);
            newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
            break;
        }
        case 2: {
            audioProcessor.suspendProcessing(true);
            // FIFOs allocation for midFilter fft analyzer
            audioProcessor.midLeftChannelFifo = new SingleChannelSampleFifo<juce::AudioBuffer<float>>{ Channel::Left };
            audioProcessor.midRightChannelFifo = new SingleChannelSampleFifo<juce::AudioBuffer<float>>{ Channel::Right };
            DSPModule* filterDSPModule = new FilterModuleDSP(audioProcessor.apvts, "Mid");
            addModuleToDSPmodules(filterDSPModule);
            audioProcessor.prepareToPlay(audioProcessor.getSampleRate(), audioProcessor.getNumSamples());
            audioProcessor.suspendProcessing(false);
            GUIModule* filterGUIModule = new FilterModuleGUI(audioProcessor, "Mid");
            addModuleToGUI(filterGUIModule);
            // NewModule reset and hide
            newModuleSelector.setSelectedId(999);
            newModuleSelector.setVisible(false);
            newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
            break;
        }

        case 3: {
            GUIModule* waveshaperGUIModule = new WaveshaperModuleGUI(audioProcessor);
            addModuleToGUI(waveshaperGUIModule);
            audioProcessor.suspendProcessing(true);
            DSPModule* waveshaperDSPModule = new WaveshaperModuleDSP(audioProcessor.apvts);
            addModuleToDSPmodules(waveshaperDSPModule);
            audioProcessor.prepareToPlay(audioProcessor.getSampleRate(), audioProcessor.getNumSamples());
            audioProcessor.suspendProcessing(false);
            // NewModule reset and hide
            newModuleSelector.setSelectedId(999);
            newModuleSelector.setVisible(false);
            newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
            break;
        }
        case 4: {
            // TODO : replace of newModule component with the selected component
            newModuleSelector.setSelectedId(999);
            newModuleSelector.setVisible(false);
            newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
            editor.updateGUI();
            break;
        }
        case 5: {
            break;
        }
        default: {
            break;
        }
        }
    };

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

    juce::Rectangle<int> newModuleBounds, deleteModuleBounds;
    newModuleBounds = deleteModuleBounds = bounds;
    newModuleBounds.reduce(20, 44);
    deleteModuleBounds.reduce(30, 44);

    auto chainPositionLabelArea = bounds.removeFromTop(bounds.getHeight() * (1.f / 4.f));

    chainPositionLabel.setBounds(chainPositionLabelArea);
    chainPositionLabel.setJustificationType(juce::Justification::centred);

    newModule.setBounds(newModuleBounds);
    newModule.setCentreRelative(0.5f, 0.5f);

    deleteModule.setBounds(deleteModuleBounds);
    deleteModule.setCentreRelative(0.75f, 0.2f);
}

std::vector<juce::Component*> NewModuleGUI::getComps()
{
    return {
        &newModule,
        &newModuleSelector
    };
}

void NewModuleGUI::setupNewModuleColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    laf.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00b5f6));

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void NewModuleGUI::setupDeleteModuleColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    //laf.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00b5f6));

    //laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    //laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void NewModuleGUI::setupCurrentModuleActivatorColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    //laf.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00b5f6));

    //laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    //laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}

void NewModuleGUI::addModuleToGUI(GUIModule* module)
{
    editor.currentGUIModule = std::unique_ptr<GUIModule>(module);
    editor.addAndMakeVisible(module);
    editor.resized();
}

void NewModuleGUI::addModuleToDSPmodules(DSPModule* module)
{
    module->setChainPosition(getChainPosition());
    bool inserted = false;
    for (auto it = audioProcessor.DSPmodules.begin(); !inserted ; ++it) {
        // end = 8° grid cell
        if ((**it).getChainPosition() == 8) {
            inserted = true;
            it = audioProcessor.DSPmodules.insert(it, std::unique_ptr<DSPModule>(module));
            continue;
        }
        // there is at least one module in the vector
        auto next = it;
        ++next;
        if ((**it).getChainPosition() < getChainPosition() && getChainPosition() < (**next).getChainPosition()) {
            inserted = true;
            it = audioProcessor.DSPmodules.insert(next, std::unique_ptr<DSPModule>(module));
        }
        // else continue to iterate to find the right grid position
    }
}

//unsigned int NewModuleGUI::addModuleToGUImodules(GUIModule* module)
//{
//    bool inserted = false;
//    unsigned int index;
//
//    for (auto it = editor.GUImodules.begin(); !inserted && it < editor.GUImodules.end(); ++it) {
//        // there are no GUImodules in the vector => first module to insert
//        // end = 8° grid cell
//        if ((**it).getGridPosition() == 8) {
//            inserted = true;
//            it = editor.GUImodules.insert(it, std::unique_ptr<GUIModule>(module));
//            index = it - editor.GUImodules.begin();
//            continue;
//        }
//        // there is at least one module in the vector
//        auto next = it;
//        ++next;
//        if ((**it).getGridPosition() < getGridPosition() && getGridPosition() < (**next).getGridPosition()) {
//            inserted = true;
//            it = editor.GUImodules.insert(next, std::unique_ptr<GUIModule>(module));
//            index = it - editor.GUImodules.begin();
//        }
//        // else continue to iterate to find the right grid position
//    }
//    this->setVisible(false);
//    newModuleSelector.setSelectedId(999);
//    newModuleSelector.setVisible(false);
//    newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
//    editor.resized();
//    return index;
//}
//
//void NewModuleGUI::addModuleToDSPmodules(DSPModule* module, unsigned int index)
//{
//    auto it = audioProcessor.DSPmodules.begin();
//    for (int i = 0; i < index; ++i) {
//        ++it;
//    }
//    audioProcessor.DSPmodules.insert(it, std::unique_ptr<DSPModule>(module));
//}
