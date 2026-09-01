/*
  ==============================================================================

    FFTAnalyzerComponent.h

    Copyright (c) 2021 KillBizz - Gabriel Bizzo

  ==============================================================================
*/

/*
  ==============================================================================

    Copyright (c) 2021 Charles Schiermeyer
    Content: the original FFT Analyzer Component
    Source: https://github.com/matkatmusic/SimpleEQ

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

#pragma once

#include <JuceHeader.h>

#include "../Shared/FFTAnalyzer.h"
#include "../Shared/PluginState.h"

struct FFTAnalyzerComponent : juce::Component,
    juce::Timer {

    // Everything that differs between the analyzer embedded in the equalizer and the standalone
    // Spectrum Analyzer module, so both can share this component instead of duplicating it.
    struct Options {
        // module owning the FFT FIFO this component reads from
        ModuleType moduleType = ModuleType::Equalizer;
        juce::Colour leftColour = juce::Colours::cyan;
        juce::Colour rightColour = juce::Colours::lightcyan;
        juce::Colour stereoColour = juce::Colour(0u, 210u, 230u);
        // the equalizer overlays the +/-24 dB filter gain scale used by its response curve;
        // a plain analyzer shows a dBFS scale instead
        bool showFilterGainScale = true;
        // fill the area below the spectrum with a soft gradient
        bool fillUnderCurve = false;
    };

    FFTAnalyzerComponent(PluginState& p, unsigned int parameterNumber, Options analyzerOptions = {});

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    PathProducer& getLeftPathProducer();
    PathProducer& getRightPathProducer();
    void toggleFFTanaysis(bool shouldEnableFFTanalysis);
    void setStereoView(bool shouldShowStereo);

    // Freezing keeps the last spectrum on screen but stops feeding new data into it (unlike
    // toggleFFTanaysis, which also clears the display).
    void setFrozen(bool shouldFreeze);

    // 1.0 = whole range visible. Horizontal zoom magnifies the frequency axis, vertical zoom
    // the dB axis (anchored at 0 dB). Panning is done by dragging the graph.
    void setViewZoom(float horizontalZoom, float verticalZoom);
    void resetView();

    // Called when the user zooms with the wheel or a trackpad pinch, so the owning module can
    // push the new values into its H/V zoom parameters. Not called by setViewZoom().
    std::function<void(float horizontalZoom, float verticalZoom)> onViewZoomChangedByGesture;

    // drag or two finger scroll pans the frequency axis, wheel and pinch zoom it
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMagnify(const juce::MouseEvent& event, float scaleFactor) override;

    // dB range the analyzer paths are mapped over (matches PathProducer's negative infinity)
    static constexpr float minimumDb = -48.0f;

private:

    PluginState& pluginState;
    Options options;

    bool enableFFTanalysis = true;
    bool stereoView = false;
    bool frozen = false;

    // view transform applied to both the spectrum paths and the grid
    float horizontalZoom = 1.0f;
    float verticalZoom = 1.0f;
    float panPixels = 0.0f;
    float panPixelsY = 0.0f;
    float panPixelsAtDragStart = 0.0f;
    float panPixelsYAtDragStart = 0.0f;

    // for the FFT analyzer
    PathProducer leftPathProducer, rightPathProducer;
    AnalyzerPathGenerator<juce::Path> stereoPathGenerator;
    juce::Path stereoFFTPath;

    void drawSpectrum(juce::Graphics& g, const juce::Path& spectrumPath,
        juce::Rectangle<int> area, juce::Colour colour);

    void drawGrid(juce::Graphics& g);
    void applyGestureZoom(float horizontalZoom, float verticalZoom);
    void zoomHorizontallyAround(float horizontalZoom, float anchorX);
    void zoomVerticallyAround(float verticalZoom, float anchorY);
    void panBy(float deltaX, float deltaY);
    void updateViewCursor(bool dragging);

    // view mapping shared by the spectrum paths and the grid
    float frequencyToX(float frequency);
    float levelToY(float levelDb);
    float getMaximumPan();
    float getMaximumPanY();
    bool isPannable();
    void clampPan();
    void drawFilterGainScale(juce::Graphics& g, juce::Rectangle<int> renderArea, int fontHeight);
    void drawLevelScale(juce::Graphics& g, juce::Rectangle<int> renderArea, int fontHeight);

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysysArea();
};