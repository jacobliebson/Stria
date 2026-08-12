// Source/Graphics/Displays/SamplerPanel.h
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/SamplerEngine.h"
#include "Displays/SamplerWaveformDisplay.h"
#include "ResonatorPalette.h"
#include "ResonatorLookAndFeel.h"

// ============================================================
// SamplerPanel
//
// The full sampler UI — waveform display, file loading,
// and playback controls. Swapped in place of ResonatorAnalyzer
// when the Sampler tab is active.
// ============================================================
class SamplerPanel : public juce::Component
{
public:
    SamplerPanel (SamplerEngine& engine, double& sampleRateRef);
    ~SamplerPanel() override = default;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    void loadFile (const juce::File& file);
    void updateControlStates();

    SamplerEngine& engine;
    double&        sampleRate;

    SamplerWaveformDisplay waveformDisplay;

    // Playback mode selector
    juce::ComboBox playbackModeBox;
    juce::Label    playbackModeLabel;

    // Trigger source toggle (raw MIDI vs post-arp)
    juce::ToggleButton triggerSourceButton { "Trigger from raw MIDI" };

    // Reverse toggle
    juce::ToggleButton reverseButton { "Reverse" };

    // Trim button
    juce::TextButton trimButton {"Trim"};

    // Reset button
    juce::TextButton resetButton {"Reset"};

    // Gain knob
    juce::Slider gainKnob;
    juce::Label  gainLabel;

    // Pitch knob
    juce::Slider pitchKnob;
    juce::Label  pitchLabel;

    // File name display
    juce::Label fileNameLabel;

    // Load button (alternative to drag-and-drop)
    juce::TextButton loadButton { "Load File" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerPanel)
};
