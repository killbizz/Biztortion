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
    // filter
    filterModuleGUI(p),
    /*peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
    responseCurveComponent(p), 
    filterFftAnalyzerComponent(p),
    peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
    peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),*/
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

    // labels
    /*waveshaperDriveLabel.setText("Drive", juce::dontSendNotification);
    waveshaperMixLabel.setText("Mix", juce::dontSendNotification);
    tanhAmpLabel.setText("Tanh Amp", juce::dontSendNotification);
    tanhSlopeLabel.setText("Tanh Slope", juce::dontSendNotification);
    sineAmpLabel.setText("Sin Amp", juce::dontSendNotification);
    sineFreqLabel.setText("Sin Freq", juce::dontSendNotification);*/
    /*peakFreqSlider.labels.add({ 0.f, "20Hz" });
    peakFreqSlider.labels.add({ 1.f, "20kHz" });
    peakGainSlider.labels.add({ 0.f, "-24dB" });
    peakGainSlider.labels.add({ 1.f, "+24dB" });
    peakQualitySlider.labels.add({ 0.f, "0.1" });
    peakQualitySlider.labels.add({ 1.f, "10.0" });
    lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20kHz" });
    highCutFreqSlider.labels.add({ 0.f, "20Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20kHz" });
    lowCutSlopeSlider.labels.add({ 0.0f, "12" });
    lowCutSlopeSlider.labels.add({ 1.f, "48" });
    highCutSlopeSlider.labels.add({ 0.0f, "12" });
    highCutSlopeSlider.labels.add({ 1.f, "48" });*/

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    setSize (700, 600);
    setResizable(true, true);
    // max limit = 4K dimension
    setResizeLimits(500, 400, 3840, 2160);
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

    /*g.setColour(juce::Colours::grey);
    g.setFont(14);
    g.drawFittedText("LowCut", lowCutSlopeSlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("Peak", peakQualitySlider.getBounds(), juce::Justification::centredBottom, 1);
    g.drawFittedText("HighCut", highCutSlopeSlider.getBounds(), juce::Justification::centredBottom, 1);*/
    
}

void BiztortionAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds().removeFromBottom(getLocalBounds().getHeight() - 32);

    // filters
    filterModuleGUI.setBounds(bounds);
    /*auto filtersArea = bounds.removeFromTop(bounds.getHeight() * (2.f / 3.f));
    auto responseCurveArea = filtersArea.removeFromTop(filtersArea.getHeight() * (1.f / 2.f));
    auto lowCutArea = filtersArea.removeFromLeft(filtersArea.getWidth() * (1.f / 3.f));
    auto highCutArea = filtersArea.removeFromRight(filtersArea.getWidth() * (1.f / 2.f));

    filterFftAnalyzerComponent.setBounds(responseCurveArea);
    responseCurveComponent.setBounds(responseCurveArea);
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * (1.f / 2.f)));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * (1.f / 2.f)));
    peakFreqSlider.setBounds(filtersArea.removeFromTop(filtersArea.getHeight() * 0.33));
    peakGainSlider.setBounds(filtersArea.removeFromTop(filtersArea.getHeight() * 0.5));
    peakQualitySlider.setBounds(filtersArea);
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutSlopeSlider.setBounds(highCutArea);*/

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
        // filter
        &filterModuleGUI,
        //&peakFreqSlider,
        //&peakGainSlider,
        //&peakQualitySlider,
        //&lowCutFreqSlider,
        //&highCutFreqSlider,
        //&lowCutSlopeSlider,
        //&highCutSlopeSlider,
        //// responseCurve
        //&filterFftAnalyzerComponent,
        //&responseCurveComponent,
        // fft analyzer
        &analyzerComponent,
        // oscilloscope
        &(audioProcessor.oscilloscope),
        // waveshaper
        &transferFunctionGraph,
        &waveshaperDriveSlider,
        &waveshaperMixSlider,
        &tanhAmpSlider,
        &tanhSlopeSlider,
        &sineAmpSlider,
        &sineFreqSlider,
        /*&waveshaperDriveLabel,
        &waveshaperMixLabel,
        &tanhAmpLabel,
        &tanhSlopeLabel,
        &sineAmpLabel,
        &sineFreqLabel*/
    };
}
