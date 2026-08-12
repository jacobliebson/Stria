// Source/Graphics/Displays/SamplerWaveformDisplay.h
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../ResonatorPalette.h"
#include "../../DSP/SamplerEngine.h"

// ============================================================
// SamplerWaveformDisplay
//
// Draws the loaded sample waveform with:
//   - A scrolling playhead indicating current position
//   - Draggable start/end point handles
//   - Drag-and-drop file loading
// ============================================================
class SamplerWaveformDisplay : public juce::Component,
                                public juce::FileDragAndDropTarget,
                                private juce::Timer
{
public:
    explicit SamplerWaveformDisplay (SamplerEngine& engine);
    ~SamplerWaveformDisplay() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Called by SamplerPanel when a new file is loaded so we can
    // rebuild the waveform thumbnail
    void sampleLoaded (const juce::AudioBuffer<float>* buffer);

    void rebuildWaveformPath();

    // FileDragAndDropTarget
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    // Callback invoked when the user drops a valid audio file
    std::function<void (juce::File)> onFileDropped;

    // Callbacks invoked when the user drags the start/end handles
    std::function<void (float)> onStartPointChanged;
    std::function<void (float)> onEndPointChanged;

    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseUp    (const juce::MouseEvent&) override;


private:
    void timerCallback() override;
    

    juce::Rectangle<float> getInnerBounds() const;
    float normalisedXToPixel (float normX) const;
    float pixelToNormalisedX (float pixelX) const;

    enum class DragTarget { None, StartHandle, EndHandle };

    SamplerEngine& engine;

    // Start and end position of audio file - update on trim
    std::atomic<float> trimStartPos;
    std::atomic<float> trimEndPos;
    
    // Visual positions of handles - update on trim
    std::atomic<float> startHandlePos;
    std::atomic<float> endHandlePos;

    float trimmedToFull (float trimmedX);

    // Downsampled peak data for drawing — rebuilt on load
    std::vector<float> waveformPeaks;
    juce::Path         waveformPath;
    bool               waveformDirty = true;

    DragTarget activeDrag = DragTarget::None;

    static constexpr int   repaintRateHz  = 30;
    static constexpr float handleWidth    = 6.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerWaveformDisplay)
};
