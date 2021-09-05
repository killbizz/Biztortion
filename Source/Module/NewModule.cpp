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
            // TODO : replace of newModule component with the selected component
            GUIModule* waveshaperModule = new WaveshaperModuleGUI(audioProcessor, getGridPosition());
            // find *this in the editor modules vector
            std::vector<std::unique_ptr<GUIModule>>::iterator newModule;
            for (auto it = editor.modules.begin(); it < editor.modules.end(); ++it) {
                auto temp = dynamic_cast<NewModuleGUI*>(&**it);
                if (temp && temp->getGridPosition() == this->getGridPosition()) {
                    newModule = it;
                }
            }
            //auto it = std::find(editor.modules.begin(), editor.modules.end(), newModule);
            // insert the newModule and erase the previous one
            editor.modules.insert(newModule, std::unique_ptr<GUIModule>(waveshaperModule));
            for (auto it = editor.modules.begin(); it < editor.modules.end(); ++it) {
                auto temp = dynamic_cast<NewModuleGUI*>(&**it);
                if (temp && temp->getGridPosition() == this->getGridPosition()) {
                    newModule = it;
                }
            }
            // TODO : modifico questa istruzione, elimina completamente *this e avviene un'eccezione
            editor.modules.erase(newModule);
            //std::replace(editor.modules.begin(), editor.modules.end(), *it, std::unique_ptr<GUIModule>(waveshaperModule));
            //editor.modules.push_back(std::unique_ptr<GUIModule>(waveshaperModule));
            //newModuleSelector.setSelectedId(999);
            //newModuleSelector.setVisible(false);
            //newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
            editor.updateGUI();
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
