#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../DSP/SamplerEngine.h"
#include "Displays/SamplerWaveformDisplay.h"
#include "ResonatorPalette.h"
#include "ResonatorLookAndFeel.h"

class SamplerPanel : public juce::Component,
                      private juce::AudioProcessorValueTreeState::Listener,
                      private juce::ChangeListener
{
public:
    SamplerPanel (SamplerEngine& engine, double& sampleRateRef, juce::AudioProcessorValueTreeState& apvts);
    ~SamplerPanel() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    void loadFile (const juce::File& file);
    void updateControlStates();

    // Forwards a parameter change (from the UI, host automation, or a preset
    // load) into the engine. This is the single place engine state gets set
    // from — nothing else should call the engine's setters directly.
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    SamplerEngine& engine;
    double&        sampleRate;
    juce::AudioProcessorValueTreeState& apvts;

    SamplerWaveformDisplay waveformDisplay;

    juce::ComboBox playbackModeBox;
    juce::Label    playbackModeLabel;

    juce::ToggleButton triggerSourceButton { "Trigger from raw MIDI" };
    juce::ToggleButton reverseButton { "Reverse" };
    juce::TextButton trimButton {"Trim"};
    juce::TextButton resetButton {"Reset"};

    juce::Slider gainKnob;
    juce::Label  gainLabel;
    juce::Slider pitchKnob;
    juce::Label  pitchLabel;

    juce::Label fileNameLabel;
    juce::TextButton loadButton { "Load File" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment>   gainAttachment;
    std::unique_ptr<SliderAttachment>   pitchAttachment;
    std::unique_ptr<ComboBoxAttachment> playbackModeAttachment;
    std::unique_ptr<ButtonAttachment>   reverseAttachment;

    static constexpr const char* gainParamID         = "SAMPLE_GAIN";
    static constexpr const char* pitchParamID        = "SAMPLE_PITCH";
    static constexpr const char* playbackModeParamID = "SAMPLE_PLAYBACK_MODE";
    static constexpr const char* reverseParamID      = "SAMPLE_REVERSE";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerPanel)
};