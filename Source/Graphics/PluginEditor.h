// Source/PluginEditor.h
#pragma once

// C++ Libraries
#include <memory>
#include <string>
#include <string_view>

// Display modules
#include "Displays/ArpDisplay.h"
#include "Displays/EnvelopeDisplay.h"
#include "Displays/ResonatorAnalyzer.h"
#include "Displays/TriggerDisplay.h"
//#include "Displays/SamplerWaveformDisplay.h"


// Graphics helpers
#include "ResonatorLookAndFeel.h"
#include "ResonatorPalette.h"
#include "SamplerPanel.h"

#include "../DSP/PluginProcessor.h"
#include "../DSP/SamplerEngine.h"






class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Timer
{
public:
    AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Helper to configure and add a labelled knob
    void setupKnob (juce::Slider& slider, juce::Label& label,
                    const juce::String& labelText,
                    std::unique_ptr<SliderAttachment>& attachment,
                    const juce::String& paramId,
                    std::string suffix="");

    // Helper to configure and add a discrete knob (integer steps)
    void setupDiscreteKnob (juce::Slider& slider, juce::Label& label,
                             const juce::String& labelText,
                             std::unique_ptr<SliderAttachment>& attachment,
                             const juce::String& paramId);

    // Helper to draw a labelled panel background
    void drawPanel (juce::Graphics& g, juce::Rectangle<int> bounds,
                    const juce::String& title);

    // Shows the sampler panel or the resonator analyzer (never both) in the
    // shared bounds they occupy, depending on the current AUDIO_SOURCE value.
    void updateAudioSourceVisibility();

    // Audio source selector (sampler vs. live input), sits in the empty area
    // of the header. Always visible regardless of which panel — sampler
    // panel or resonator analyzer — is currently shown, since it's the
    // control used to switch between them.
    juce::ComboBox audioSourceBox;
    juce::Label    audioSourceLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> audioSourceAttachment;

    AudioPluginAudioProcessor& audioProcessor;
    ResonatorLookAndFeel lookAndFeel;

    //==========================================================================
    // Resonator panel
    juce::Slider feedbackKnob,  dampingKnob, detuneKnob, detuneModeKnob;
    juce::Label  feedbackLabel, dampingLabel, detuneLabel, detuneModeLabel;
    std::unique_ptr<SliderAttachment> feedbackAttachment, dampingAttachment, detuneAttachment, detuneModeAttachment;

    //==========================================================================
    // Trigger panel
    juce::Slider trigThresholdKnob,  trigAttackKnob,  trigHoldKnob, trigReleaseKnob;
    juce::Label  trigThresholdLabel, trigAttackLabel, trigHoldLabel, trigReleaseLabel;
    std::unique_ptr<SliderAttachment> trigThresholdAttachment, trigAttackAttachment, 
                                      trigHoldAttachment, trigReleaseAttachment;

    std::unique_ptr<TriggerDisplay> triggerDisplay;

    juce::TextButton legatoButton { "Legato" };
    juce::TextButton retriggerButton   { "Retrigger"   };
    bool legatoMode = false;
    void switchTriggerModeTo (bool legato);

    //==========================================================================
    // Analyzer panel
    std::unique_ptr<ResonatorAnalyzer> resonatorAnalyzer;

    // Sampler panel
    std::unique_ptr<SamplerPanel> samplerPanel;
    
    // Envelope panel
    std::unique_ptr<EnvelopeDisplay> envelopeDisplay;

    // Tab selector — switches between chord and arp envelope parameter sets
    juce::TextButton chordEnvButton { "Chord" };
    juce::TextButton arpEnvButton   { "Arp"   };
    bool showingArpEnv = false;
    void switchEnvelopeTo (bool showArp);

    juce::Slider attackKnob,  decayKnob,  sustainKnob,  releaseEnvKnob;
    juce::Label  attackLabel, decayLabel, sustainLabel, releaseEnvLabel;
    std::unique_ptr<SliderAttachment> attackAttachment, decayAttachment,
                                      sustainAttachment, releaseEnvAttachment;

    //==========================================================================
    // Arpeggiator panel
    std::unique_ptr<ArpDisplay> arpDisplay;

    juce::Slider rateKnob,  gateKnob,  modeKnob,  deviationKnob,  scatterKnob,  octaveRangeKnob;
    juce::Label  rateLabel, gateLabel, modeLabel, deviationLabel, scatterLabel, octaveRangeLabel;
    std::unique_ptr<SliderAttachment> rateAttachment, gateAttachment, modeAttachment,
                                      deviationAttachment, scatterAttachment, octaveRangeAttachment;

    //==========================================================================
    // Mixer panel
    juce::Slider arpGainSlider,  chordGainSlider,  mixSlider, spreadKnob, arpPanKnob, chordPanKnob;
    juce::Label  arpGainLabel,   chordGainLabel,   mixLabel, spreadLabel, arpPanLabel, chordPanLabel;
    std::unique_ptr<SliderAttachment> arpGainAttachment, chordGainAttachment, 
                                      mixAttachment, spreadAttachment, arpPanAttachment, chordPanAttachment;

    //==========================================================================
    // Bypass toggle (global, outside panels)
    //juce::ToggleButton bypassButton;
    //std::unique_ptr<ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};