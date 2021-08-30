/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BiztortionAudioProcessorEditor::BiztortionAudioProcessorEditor (BiztortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), analyzerComponent(p), newModule("+"),
    //filterModuleGUI(p),
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

    newModuleSelector.addItem("Select one module here", 999);
    newModuleSelector.addItem("Spectrum Analyzer", 1);
    newModuleSelector.addItem("Oscilloscope", 2);
    newModuleSelector.addItem("Filters", 3);
    newModuleSelector.addItem("Waveshaper", 4);

    newModuleSelector.setSelectedId(999);

    // TODO : close newModuleSelector if newModule toggle loses focus
    // TODO : customize toggle
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
                juce::Component* filterModule = new FilterModuleGUI(audioProcessor);
                modules.push_back(std::unique_ptr<Component>(filterModule));
                // filterModule->setBounds(availableBounds.removeFromLeft(200));
                newModuleSelector.setSelectedId(999);
                newModuleSelector.setVisible(false);
                newModule.setToggleState(false, juce::NotificationType::dontSendNotification);
                updateGUI();
                break;
            }
            default: {
                break;
            }
        }
    };

    juce::Component* filterModule = new FilterModuleGUI(audioProcessor);
    //juce::Component* filterModule2 = new FilterModuleGUI(audioProcessor);
    addAndMakeVisible(filterModule);
    //addAndMakeVisible(filterModule2);
    modules.push_back(std::unique_ptr<Component>(filterModule));
    //modules.push_back(std::unique_ptr<Component>(filterModule2));

    // labels
    /*waveshaperDriveLabel.setText("Drive", juce::dontSendNotification);
    waveshaperMixLabel.setText("Mix", juce::dontSendNotification);
    tanhAmpLabel.setText("Tanh Amp", juce::dontSendNotification);
    tanhSlopeLabel.setText("Tanh Slope", juce::dontSendNotification);
    sineAmpLabel.setText("Sin Amp", juce::dontSendNotification);
    sineFreqLabel.setText("Sin Freq", juce::dontSendNotification);*/

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    addChildComponent(newModuleSelector);

    setSize (700, 600);
    setResizable(true, true);
    // max limit = 4K dimension
    setResizeLimits(400, 332, 3840, 2160);
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
    
}

void BiztortionAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    juce::FlexBox fb;
    fb.flexWrap = juce::FlexBox::Wrap::wrap;
    fb.justifyContent = juce::FlexBox::JustifyContent::spaceBetween;
    fb.alignContent = juce::FlexBox::AlignContent::center;

    for (auto it = modules.cbegin(); it < modules.cend(); ++it)
    {
        auto item = juce::FlexItem(**it);
        item.flexGrow = 1;
        item.flexShrink = 1;
        fb.items.add(item.withMinWidth(400.0f).withMinHeight(300.0f).withMaxWidth(700.0f).withMaxHeight(600.0f));
    }
    fb.items.add(juce::FlexItem(newModule).withMinWidth(100.0f).withMinHeight(100.0f).withMaxWidth(100.0f).withMaxHeight(100.0f));
    fb.items.add(juce::FlexItem(newModuleSelector).withMinWidth(100.0f).withMinHeight(100.0f).withMaxWidth(100.0f).withMaxHeight(100.0f));

    // 32 = header height
    fb.performLayout(getLocalBounds().removeFromBottom(getLocalBounds().getHeight() - 32).toFloat());

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
        &newModule,
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
    resized();
}
