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

NewModuleGUI::NewModuleGUI(BiztortionAudioProcessor& p, BiztortionAudioProcessorEditor& e, unsigned int gridPosition)
    : GUIModule(gridPosition), audioProcessor(p), editor(e)
{
    addAndMakeVisible(newModule);
    newModule.setClickingTogglesState(true);
    newModule.setToggleState(false, juce::dontSendNotification);

    setupCustomLookAndFeelColours(lookAndFeel);
    newModule.setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(newModuleSelector);
    newModuleSelector.addItem("Select one module here", 999);
    newModuleSelector.addItem("Oscilloscope", 1);
    newModuleSelector.addItem("Filter", 2); // ?
    newModuleSelector.addItem("Waveshaper", 3);
    newModuleSelector.addItem("Bitcrusher", 4);
    newModuleSelector.addItem("Clipper", 5);

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
            //GUIModule* oscilloscopeGUI = new OscilloscopeModuleGUI(audioProcessor);
            // replace of the newModule with the oscilloscopeGUI in the desired position of the grid
            break;
        }
        case 2: {
            // REMEMBER TO INSTANTIATE THE RELATIVE fifos in the PluginProcessor for fft analyzer

            //GUIModule* midFilter = new FilterModuleGUI(audioProcessor, "Mid");
            //editor.modules.push_back(std::unique_ptr<GUIModule>(midFilter));

            // TODO : replace of newModule component with the selected component
            newModuleSelector.setSelectedId(999);
            newModuleSelector.setVisible(false);
            newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
            editor.updateGUI();
            break;
        }

        case 3: {
            GUIModule* waveshaperModule = new WaveshaperModuleGUI(audioProcessor, getGridPosition());

            bool inserted = false;
            for (auto it = editor.GUImodules.begin(); !inserted && it < editor.GUImodules.end(); ++it) {
                // there are no GUImodules in the vector => first module to insert
                // end = 8° grid cell
                if ((**it).getGridPosition() == 8) {
                    inserted = true;
                    editor.GUImodules.insert(it, std::unique_ptr<GUIModule>(waveshaperModule));
                    continue;
                }
                // there is at least one module in the vector
                auto next = it;
                ++next;
                if ((**it).getGridPosition() < getGridPosition() && getGridPosition() <(**next).getGridPosition()) {
                    inserted = true;
                    it = editor.GUImodules.insert(next, std::unique_ptr<GUIModule>(waveshaperModule));
                }
                // else continue to iterate to find the right grid position
            }
            this->setVisible(false);
            newModuleSelector.setSelectedId(999);
            newModuleSelector.setVisible(false);
            newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
            editor.resized();
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

}

void NewModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void NewModuleGUI::resized()
{
    auto bounds = getContentRenderArea();
    bounds.reduce(140, 100);
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
