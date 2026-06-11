// Source/PluginEditor.h
#pragma once
#include "PluginProcessor.h"
#include "ResonatorLookAndFeel.h"
#include "ArpDisplay.h"
#include "EnvelopeDisplay.h"
#include "ResonatorAnalyzer.h"
#include "TriggerDisplay.h"
#include <memory>
#include <string>

class AudioPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         public juce::Timer
{
public:
    AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override {}

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
    juce::Slider arpGainSlider,  chordGainSlider,  mixKnob;
    juce::Label  arpGainLabel,   chordGainLabel,   mixLabel;
    std::unique_ptr<SliderAttachment> arpGainAttachment, chordGainAttachment, mixAttachment;

    //==========================================================================
    // Bypass toggle (global, outside panels)
    //juce::ToggleButton bypassButton;
    //std::unique_ptr<ButtonAttachment> bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
