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

NewModuleGUI::NewModuleGUI(BiztortionAudioProcessor& p, BiztortionAudioProcessorEditor& e, unsigned int gridPosition)
    : GUIModule(gridPosition), audioProcessor(p), editor(e)
{
    addAndMakeVisible(newModule);
    newModule.setClickingTogglesState(true);
    newModule.setToggleState(false, juce::dontSendNotification);

    setupCustomLookAndFeelColours(lookAndFeel);
    //lookAndFeel = new CustomLookAndFeel();
    newModule.setLookAndFeel(&lookAndFeel);

    newModuleSelector.addItem("Select one module here", 999);
    newModuleSelector.addItem("Spectrum Analyzer", 1);
    newModuleSelector.addItem("Oscilloscope", 2);
    newModuleSelector.addItem("Filters", 3);
    newModuleSelector.addItem("Waveshaper", 4);

    newModuleSelector.setSelectedId(999);

    // TODO : close newModuleSelector if newModule toggle loses focus

    newModule.onClick = [this] {
        newModuleSelector.setVisible(newModule.getToggleState());
        auto newModuleBounds = newModule.getBounds();
        newModuleBounds.setX(newModuleBounds.getRight() + newModuleBounds.getWidth() / 2);
        newModuleSelector.setBounds(newModuleBounds);
    };

    newModuleSelector.onChange = [this] {
        
        switch (newModuleSelector.getSelectedId()) {
        case 1: {
            break;
        }
        case 2: {
            break;
        }

        case 3: {
            //GUIModule* midFilter = new FilterModuleGUI(audioProcessor, "Mid");
            //editor.modules.push_back(std::unique_ptr<GUIModule>(midFilter));
            
            // TODO : replace of newModule component with the selected component
            newModuleSelector.setSelectedId(999);
            newModuleSelector.setVisible(false);
            newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
            editor.updateGUI();
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

}

void NewModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void NewModuleGUI::resized()
{
    auto bounds = getContentRenderArea();
    bounds.reduce(100, 100);
    newModule.setBounds(bounds);
    newModule.setCentreRelative(0.5f, 0.5f);
}

std::vector<juce::Component*> NewModuleGUI::getComps()
{
    return {
        &newModule,
        &newModuleSelector
    };
}

void NewModuleGUI::setupCustomLookAndFeelColours(juce::LookAndFeel& laf)
{
    laf.setColour(juce::TextButton::buttonColourId, juce::Colours::white);
    laf.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00b5f6));

    laf.setColour(juce::TextButton::buttonOnColourId, laf.findColour(juce::TextButton::textColourOffId));
    laf.setColour(juce::TextButton::textColourOnId, laf.findColour(juce::TextButton::buttonColourId));
}