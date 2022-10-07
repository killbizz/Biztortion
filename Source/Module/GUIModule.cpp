/*
  ==============================================================================

    GUIModule.cpp

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

#include "GUIModule.h"

GUIModule::GUIModule() : remapper(0.f, 100.f, 1.f, 1.3f) // exponential remapping for a smoother lightness fx in the outline
{
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(45, 70); // define the range

    brightnessCounter = distr(gen);

    startTimerHz(10);
}

void GUIModule::drawContainer(juce::Graphics& g)
{

    // container border and background
    g.setColour(juce::Colour(132, 135, 138));
    g.drawRoundedRectangle(getContainerArea().toFloat(), 4.f, 1.f);
    g.fillRoundedRectangle(getContainerArea().toFloat(), 4.f);

    // content border
    g.setColour(juce::Colours::black);
    auto renderArea = getContentRenderArea().toFloat();
    g.drawRoundedRectangle(renderArea.toFloat(), 4.f, 1.f);

    // module outline
    renderArea.expand(5.5, 5.5);
    moduleColor = moduleColor.withLightness(remapper.convertTo0to1((float)brightnessCounter));
    g.setColour(moduleColor);
    g.drawRoundedRectangle(renderArea, 4.f, 4.f);

    if (brightnessCounter == 70) {
        addingFactor = -1;
    }
    else if (brightnessCounter == 45) {
        addingFactor = 1;
    }
    brightnessCounter = brightnessCounter + addingFactor;

}

juce::Rectangle<int> GUIModule::getContentRenderArea()
{
    auto bounds = getContainerArea();
    bounds.reduce(10, 10);

    return bounds;
}

void GUIModule::handleParamCompsEnablement(bool bypass)
{
    for (auto* comp : getParamComps())
    {
        comp->setEnabled(!bypass);
    }
}

void GUIModule::paint(juce::Graphics& g)
{
    drawContainer(g);
}

void GUIModule::timerCallback()
{
    repaint();
}

juce::Rectangle<int> GUIModule::getContainerArea()
{
    // returns a dimesion reduced rectangle as bounds in order to avoid margin collisions
    auto bounds = getLocalBounds();
    bounds.reduce(5, 5);

    return bounds;
}