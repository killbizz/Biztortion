/*
  ==============================================================================

    WelcomeModule.cpp

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

#include "WelcomeModule.h"

WelcomeModuleGUI::WelcomeModuleGUI()
{
    bigNose = juce::Drawable::createFromImageData(BinaryData::bigNose1_svg, BinaryData::bigNose1_svgSize);
    addAndMakeVisible(*bigNose);
}

void WelcomeModuleGUI::paint(juce::Graphics& g)
{
    drawContainer(g);
    //bigNose->draw(g, 1.f, getTransform());
}

void WelcomeModuleGUI::resized()
{
    bigNose->setTransform(getTransform());
    bigNose->setCentreRelative(0.5f, 0.5f);
    //bigNose->setBounds(getContentRenderArea());
}

std::vector<juce::Component*> WelcomeModuleGUI::getComps()
{
    return {
        &*bigNose
    };
}

juce::AffineTransform WelcomeModuleGUI::getTransform()
{
    return juce::AffineTransform::scale(0.7f, 0.45f);
}
