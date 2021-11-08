/*
  ==============================================================================

    PluginEditor.cpp

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

/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BiztortionAudioProcessorEditor::BiztortionAudioProcessorEditor (BiztortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), githubLink("GitHub", {"https://github.com/killbizz"})
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    currentGUIModule = std::unique_ptr<GUIModule>(new WelcomeModuleGUI());
    addAndMakeVisible(*currentGUIModule);

    githubLink.setFont(20, true);
    githubLink.setColour(juce::HyperlinkButton::textColourId, juce::Colours::white);
    addAndMakeVisible(githubLink);

    // 1 - 8 = chain positions with visible components in the chain part of the UI
    for (int i = 0; i < 8; ++i) {
        NewModuleGUI* item = new NewModuleGUI(audioProcessor, *this, i + 1);
        newModules.push_back(std::unique_ptr<NewModuleGUI>(item));
    }

    inputMeter = std::unique_ptr<GUIModule>(new MeterModuleGUI(audioProcessor, "Input"));
    addAndMakeVisible(*inputMeter);
    outputMeter = std::unique_ptr<GUIModule>(new MeterModuleGUI(audioProcessor, "Output"));
    addAndMakeVisible(*outputMeter);

    for (auto it = newModules.rbegin(); it < newModules.rend(); ++it)
    {
        addAndMakeVisible(**it);
    }

    editorSetup();

    setSize(900, 577);
    setResizable(false, false);
    // setResizeLimits(400, 332, 3840, 2160);
}

BiztortionAudioProcessorEditor::~BiztortionAudioProcessorEditor()
{
}

//==============================================================================
void BiztortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);

    Path curve;

    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();

    g.setFont(Font("Prestige Elite Std", 30, 0));

    String title{ "Biztortion" };
    g.setFont(30);
    auto titleWidth = g.getCurrentFont().getStringWidth(title);

    curve.startNewSubPath(center.x, 32);
    curve.lineTo(center.x - titleWidth * 0.45f, 32);

    auto cornerSize = 20;
    auto curvePos = curve.getCurrentPosition();
    curve.quadraticTo(curvePos.getX() - cornerSize, curvePos.getY(),
        curvePos.getX() - cornerSize, curvePos.getY() - 16);
    curvePos = curve.getCurrentPosition();
    curve.quadraticTo(curvePos.getX(), 2,
        curvePos.getX() - cornerSize, 2);

    curve.lineTo({ 0.f, 2.f });
    curve.lineTo(0.f, 0.f);
    curve.lineTo(center.x, 0.f);
    curve.closeSubPath();

    g.setColour(juce::Colours::gainsboro);
    g.fillPath(curve);

    curve.applyTransform(AffineTransform().scaled(-1.01, 1));
    curve.applyTransform(AffineTransform().translated(getWidth(), 0));
    g.fillPath(curve);


    g.setColour(juce::Colours::black);
    g.drawFittedText(title, bounds, juce::Justification::centredTop, 1);
    
}

void BiztortionAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();

    githubLink.setBoundsRelative(0.75f, 0.01f, 0.1f, 0.05);

    auto chainArea = bounds.removeFromBottom(bounds.getHeight() * (1.f / 4.f));
    auto temp = bounds;
    temp = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto preStageArea = temp.removeFromLeft(temp.getWidth() * 0.3f);
    // 32 = header height
    preStageArea.removeFromTop(32);

    auto inputMeterArea = preStageArea;

    temp = bounds;
    temp = temp.removeFromRight(temp.getWidth() * (1.f / 2.f));
    auto postStageArea = temp.removeFromRight(temp.getWidth() * 0.3f);
    // 32 = header height
    postStageArea.removeFromTop(32);
    
    auto outputMeterArea = postStageArea;

    bounds.setLeft(preStageArea.getTopRight().getX());
    bounds.setRight(postStageArea.getTopLeft().getX());
    // 32 = header height
    bounds.removeFromTop(32);

    auto currentGUImoduleArea = bounds;

    // chopping the chainArea in 8 equal rectangles for 8 NewModules
    std::vector<juce::Rectangle<int>> chainAreas;
    auto width = (float) chainArea.getWidth();
    width /= 8.f;
    for (int i = 0; i < 8; ++i) {
        chainAreas.push_back(chainArea.removeFromLeft(width));
    }

    inputMeter->setBounds(inputMeterArea);
    outputMeter->setBounds(outputMeterArea);
    if(currentGUIModule)    currentGUIModule->setBounds(currentGUImoduleArea);
    auto it = newModules.begin();
    for (int i = 0; i < 8; ++i) {
        (*it)->setBounds(chainAreas[i]);
        ++it;
    }
}

void BiztortionAudioProcessorEditor::editorSetup()
{
    auto it = ++audioProcessor.DSPmodules.begin();
    auto end = --audioProcessor.DSPmodules.end();
    while ( it < end ) {
        auto newModuleIt = newModules.begin() + (**it).getChainPosition() -1;
        (**newModuleIt).newModuleSetup((**it).getModuleType());
        ++it;
    }
}
