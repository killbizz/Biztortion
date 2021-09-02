/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BiztortionAudioProcessorEditor::BiztortionAudioProcessorEditor (BiztortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), analyzerComponent(p),
    // waveshaper
    waveshaperDriveSlider(*audioProcessor.apvts.getParameter("Waveshaper Drive"), "dB"),
    waveshaperMixSlider(*audioProcessor.apvts.getParameter("Waveshaper Mix"), "%"),
    tanhAmpSlider(*audioProcessor.apvts.getParameter("Waveshaper Tanh Amp"), ""),
    tanhSlopeSlider(*audioProcessor.apvts.getParameter("Waveshaper Tanh Slope"), ""),
    sineAmpSlider(*audioProcessor.apvts.getParameter("Waveshaper Sine Amp"), ""),
    sineFreqSlider(*audioProcessor.apvts.getParameter("Waveshaper Sine Freq"), ""),
    transferFunctionGraph(p),
    waveshaperDriveSliderAttachment(audioProcessor.apvts, "Waveshaper Drive", waveshaperDriveSlider),
    waveshaperMixSliderAttachment(audioProcessor.apvts, "Waveshaper Mix", waveshaperMixSlider),
    tanhAmpSliderAttachment(audioProcessor.apvts, "Waveshaper Tanh Amp", tanhAmpSlider),
    tanhSlopeSliderAttachment(audioProcessor.apvts, "Waveshaper Tanh Slope", tanhSlopeSlider),
    sineAmpSliderAttachment(audioProcessor.apvts, "Waveshaper Sine Amp", sineAmpSlider),
    sineFreqSliderAttachment(audioProcessor.apvts, "Waveshaper Sine Freq", sineFreqSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (int i = 0; i < 6; ++i) {
        GUIModule* item = new NewModuleGUI(audioProcessor, *this, i + 2);
        modules.push_back(std::unique_ptr<GUIModule>(item));
    }

    GUIModule* inputMeter = new MeterModuleGUI(audioProcessor, "Input");
    modules.push_back(std::unique_ptr<GUIModule>(inputMeter));
    GUIModule* preFilter = new FilterModuleGUI(audioProcessor, "Pre");
    modules.push_back(std::unique_ptr<GUIModule>(preFilter));
    GUIModule* postFilter = new FilterModuleGUI(audioProcessor, "Post");
    modules.push_back(std::unique_ptr<GUIModule>(postFilter));
    GUIModule* outputMeter = new MeterModuleGUI(audioProcessor, "Output");
    modules.push_back(std::unique_ptr<GUIModule>(outputMeter));

    // labels
    /*waveshaperDriveLabel.setText("Drive", juce::dontSendNotification);
    waveshaperMixLabel.setText("Mix", juce::dontSendNotification);
    tanhAmpLabel.setText("Tanh Amp", juce::dontSendNotification);
    tanhSlopeLabel.setText("Tanh Slope", juce::dontSendNotification);
    sineAmpLabel.setText("Sin Amp", juce::dontSendNotification);
    sineFreqLabel.setText("Sin Freq", juce::dontSendNotification);*/

    updateGUI();

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
    
    auto postFilterArea = postStageArea.removeFromTop(postStageArea.getHeight() * (1.f / 3.f));
    auto outputMeterArea = postStageArea;

    bounds.setLeft(preStageArea.getTopRight().getX());
    bounds.setRight(postStageArea.getTopLeft().getX());


    juce::FlexBox fb;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    //fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    fb.justifyContent = juce::FlexBox::JustifyContent::center;
    fb.alignContent = juce::FlexBox::AlignContent::center;

    for (auto it = modules.cbegin(); it < modules.cend(); ++it)
    {
        auto gp = (**it).getGridPosition();
        if (gp > 1 && gp < 8) {
            auto item = juce::FlexItem(**it);
            //item.flexGrow = 1;
            //item.flexShrink = 1;
            fb.items.add(item.withMinWidth(350.0f).withMinHeight(250.0f).withMaxWidth(350.0f).withMaxHeight(250.0f));
        }
        else if (gp == 0)    (**it).setBounds(inputMeterArea);
        else if (gp == 1)    (**it).setBounds(preFilterArea);
        else if (gp == 8)   (**it).setBounds(postFilterArea);
        else if (gp == 9)   (**it).setBounds(outputMeterArea);
    }
    /*fb.items.add(juce::FlexItem(newModule).withMinWidth(100.0f).withMinHeight(100.0f).withMaxWidth(100.0f).withMaxHeight(100.0f));
    fb.items.add(juce::FlexItem(newModuleSelector).withMinWidth(100.0f).withMinHeight(100.0f).withMaxWidth(100.0f).withMaxHeight(100.0f));*/

    // 32 = header height
    fb.performLayout(bounds.removeFromBottom(getLocalBounds().getHeight() - 32).toFloat());

    //auto bounds = getLocalBounds().removeFromBottom(getLocalBounds().getHeight() - 32);
    /*auto bounds = getLocalBounds();
    availableBounds = bounds.removeFromRight(bounds.getWidth() * (1.f / 2.f));
    newModule.setBounds(bounds);*/

    // filters
    //filterModuleGUI.setBounds(bounds);

    // waveshaper
    // TODO : add labels
    //auto waveshaperArea = bounds.removeFromTop(bounds.getHeight() /** (1.f / 2.f)*/);
    //auto waveshaperGraphArea = waveshaperArea.removeFromLeft(waveshaperArea.getWidth() * (1.f / 2.f));
    //auto waveshaperBasicControlsArea = waveshaperArea.removeFromTop(waveshaperArea.getHeight() * (1.f / 3.f));
    //auto waveshaperTanhControlsArea = waveshaperArea.removeFromTop(waveshaperArea.getHeight() * (1.f / 2.f));

    //transferFunctionGraph.setBounds(waveshaperGraphArea);
    //waveshaperDriveSlider.setBounds(waveshaperBasicControlsArea.removeFromLeft(waveshaperBasicControlsArea.getWidth() * (1.f / 2.f)));
    //waveshaperMixSlider.setBounds(waveshaperBasicControlsArea);
    //tanhAmpSlider.setBounds(waveshaperTanhControlsArea.removeFromLeft(waveshaperTanhControlsArea.getWidth() * (1.f / 2.f)));
    //tanhSlopeSlider.setBounds(waveshaperTanhControlsArea);
    //sineAmpSlider.setBounds(waveshaperArea.removeFromLeft(waveshaperArea.getWidth() * (1.f / 2.f)));
    //sineFreqSlider.setBounds(waveshaperArea);

    // analyzer
    /*auto analyzerArea = bounds.removeFromRight(bounds.getWidth() * (1.f / 2.f));

    analyzerComponent.setBounds(analyzerArea);*/

    // oscilloscope
    //audioProcessor.oscilloscope.setBounds(bounds);
}

std::vector<juce::Component*> BiztortionAudioProcessorEditor::getComps()
{
    return {
        //&newModule,
        //&newModuleSelector,
        // filter
        //&filterModuleGUI,
        // fft analyzer
        //&analyzerComponent,
        // oscilloscope
        //&(audioProcessor.oscilloscope),
        // waveshaper
        //&transferFunctionGraph,
        //&waveshaperDriveSlider,
        //&waveshaperMixSlider,
        //&tanhAmpSlider,
        //&tanhSlopeSlider,
        //&sineAmpSlider,
        //&sineFreqSlider,
        /*&waveshaperDriveLabel,
        &waveshaperMixLabel,
        &tanhAmpLabel,
        &tanhSlopeLabel,
        &sineAmpLabel,
        &sineFreqLabel*/
    };
}

void BiztortionAudioProcessorEditor::updateGUI()
{
    for (auto it = modules.cbegin(); it < modules.cend(); ++it)
    {
        addAndMakeVisible(**it);
    }
    //resized();
}
