/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BiztortionAudioProcessorEditor::BiztortionAudioProcessorEditor (BiztortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (int i = 0; i < 6; ++i) {
        NewModuleGUI* item = new NewModuleGUI(audioProcessor, *this, i + 2);
        newModules.push_back(std::unique_ptr<NewModuleGUI>(item));
    }

    GUIModule* inputMeter = new MeterModuleGUI(audioProcessor, "Input");
    GUImodules.push_back(std::unique_ptr<GUIModule>(inputMeter));
    GUIModule* preFilter = new FilterModuleGUI(audioProcessor, "Pre", 1);
    GUImodules.push_back(std::unique_ptr<GUIModule>(preFilter));
    GUIModule* postFilter = new FilterModuleGUI(audioProcessor, "Post", 8);
    GUImodules.push_back(std::unique_ptr<GUIModule>(postFilter));
    GUIModule* outputMeter = new MeterModuleGUI(audioProcessor, "Output");
    GUImodules.push_back(std::unique_ptr<GUIModule>(outputMeter));

    updateGUI();

    for (auto it = newModules.cbegin(); it < newModules.cend(); ++it)
    {
        addAndMakeVisible(**it);
    }

    setSize (1400, 782);
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
    g.fillAll(Colour(21, 53, 79));

    Path curve;

    auto bounds = getLocalBounds();
    auto center = bounds.getCentre();

    g.setFont(Font("Iosevka Term Slab", 30, 0)); //https://github.com/be5invis/Iosevka

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

    g.setColour(Colour(97u, 18u, 167u));
    g.fillPath(curve);

    curve.applyTransform(AffineTransform().scaled(-1, 1));
    curve.applyTransform(AffineTransform().translated(getWidth(), 0));
    g.fillPath(curve);


    g.setColour(Colour(255u, 154u, 1u));
    g.drawFittedText(title, bounds, juce::Justification::centredTop, 1);

    // TODO : draw the grid vertical and horizontal lines to divide modules
    
}

void BiztortionAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    // TODO: implementare un layout stretching automatico
    // oscilloscope = modulo come gli altri distorsori
    // modalità small : pre/post-stage + pulsante per aggiungere 1 modulo
    // modalità medium : aggiungere fino a un max di 3 moduli (tutti aggiunti in colonna)
    // modalità large : aggiungere da 4 a max 6 moduli

    updateGUI();

    auto bounds = getLocalBounds();
    auto temp = bounds;
    temp = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    auto preStageArea = temp.removeFromLeft(temp.getWidth() * (1.f / 2.f));
    preStageArea.removeFromTop(32);

    auto inputMeterArea = preStageArea.removeFromTop(preStageArea.getHeight() * (1.f / 3.f));
    auto preFilterArea = preStageArea;

    temp = bounds;
    temp = temp.removeFromRight(temp.getWidth() * (1.f / 2.f));
    auto postStageArea = temp.removeFromRight(temp.getWidth() * (1.f / 2.f));
    postStageArea.removeFromTop(32);
    
    auto outputMeterArea = postStageArea.removeFromTop(postStageArea.getHeight() * (1.f / 3.f));
    auto postFilterArea = postStageArea;

    bounds.setLeft(preStageArea.getTopRight().getX());
    bounds.setRight(postStageArea.getTopLeft().getX());

    // TODO : invertire inserimento degli item da orizzontale a verticale
    // item.flexGrow = 1;
    // item.flexShrink = 1;
    // fb.items.add(item.withMinWidth(350.0f).withMinHeight(250.0f).withMaxWidth(350.0f).withMaxHeight(250.0f));

    juce::FlexBox fb;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    //fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignContent = juce::FlexBox::AlignContent::center;
    
    std::vector<std::unique_ptr<GUIModule>>::const_iterator it = GUImodules.cbegin();
    for (int i = 0; i < 10; ++i) {
        if (i == 0) {
            (**it).setBounds(inputMeterArea);
            ++it;
        }
        else if (i == 1) {
            (**it).setBounds(preFilterArea);
            ++it;
        }
        else {
            // dopo devo riempire di NewModules ( con i == (**it2).getGridPosition() ) il layout finchè non arrivo ad avere i == (**it).getGridPosition(), 
            // dove devo metterci il modulo **it
            auto gp = (**it).getGridPosition();
            juce::FlexItem item;
            while (i != gp) {
                bool found = false;
                for (auto it2 = newModules.cbegin(); !found && it2 < newModules.cend(); ++it2) {
                    if ((**it2).getGridPosition() == i) {
                        found = true;
                        item = juce::FlexItem(**it2);
                        fb.items.add(item.withMinWidth(350.0f).withMinHeight(250.0f).withMaxWidth(350.0f).withMaxHeight(250.0f));
                    }
                }
                ++i;
            }
            // i == (**it).getGridPosition() => insert the GUImodule
            if (i < 8) {
                item = juce::FlexItem(**it);
                fb.items.add(item.withMinWidth(350.0f).withMinHeight(250.0f).withMaxWidth(350.0f).withMaxHeight(250.0f));
                ++it;
            }
            else if (i == 8) {
                (**it).setBounds(postFilterArea);
                ++it;
            }
            else if (i == 9) {
                (**it).setBounds(outputMeterArea);
                ++it;
            }
        }
    }

    // 32 = header height
    fb.performLayout(bounds.removeFromBottom(getLocalBounds().getHeight() - 32).toFloat());

    // oscilloscope
    //audioProcessor.oscilloscope.setBounds(bounds);
}

//std::vector<juce::Component*> BiztortionAudioProcessorEditor::getComps()
//{
//    return {
//        //&newModule,
//        //&newModuleSelector,
//        // filter
//        //&filterModuleGUI,
//        // fft analyzer
//        //&analyzerComponent,
//        // oscilloscope
//        //&(audioProcessor.oscilloscope),
//        // waveshaper
//        //&transferFunctionGraph,
//        //&waveshaperDriveSlider,
//        //&waveshaperMixSlider,
//        //&tanhAmpSlider,
//        //&tanhSlopeSlider,
//        //&sineAmpSlider,
//        //&sineFreqSlider,
//        /*&waveshaperDriveLabel,
//        &waveshaperMixLabel,
//        &tanhAmpLabel,
//        &tanhSlopeLabel,
//        &sineAmpLabel,
//        &sineFreqLabel*/
//    };
//}

void BiztortionAudioProcessorEditor::updateGUI()
{
    for (auto it = GUImodules.cbegin(); it < GUImodules.cend(); ++it)
    {
        addAndMakeVisible(**it);
    }
    // resized();
}
