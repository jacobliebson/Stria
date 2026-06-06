#include "Parameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout Parameters::configureParameters () {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

  juce::NormalisableRange<float> feedbackRange(-1.0f, 1.0f, 0.01f, 1.0f);

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("FEEDBACK", 1),

      "Feedback",

      feedbackRange,

      0.8f

      ));

  juce::NormalisableRange<float> dampingRange(0.0f, 1.0f, 0.01f, 1.0f);

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("DAMPING", 1),

      "Damping",

      dampingRange,

      0.5f

      ));

  // Wet Signal Makeup Gain: -24 dB to +24 dB

  juce::NormalisableRange<float> wetGainRange(-24.0f, 24.0f, 0.1f);

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("WET_GAIN", 1),

      "Resonator Gain",

      wetGainRange,

      6.0f // Default to 0 dB (no change)

      ));

  juce::NormalisableRange<float> mixRange(0.0f, 100.0f, 0.1f, 1.0f);

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("MIX", 1),

      "Mix",

      mixRange,

      100.0f

      ));

  // 2. Trigger Release: 1.0ms to 500ms

  juce::NormalisableRange<float> trigReleaseRange(1.0f, 500.0f, 0.1f);

  trigReleaseRange.setSkewForCentre(50.0f); // 50ms center

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("TRIG_RELEASE", 1),

      "Trigger Release",

      trigReleaseRange,

      100.0f));

  juce::NormalisableRange<float> trigSoftnessRange(0.0f, 200.0f, 0.1f);

  trigSoftnessRange.setSkewForCentre(25.0f);

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("TRIG_SOFTNESS", 1),

      "Softness",

      trigSoftnessRange,

      0.0f));

  // 3. Trigger Sensitivity / Threshold Ratio (Linear dB scale)

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("TRIG_THRESHOLD", 1),

      "Trigger Sensitivity",

      0.0f,

      24.0f,

      6.0f));

  // 1. Env Attack: 0.001s (1ms) to 2.0s

  juce::NormalisableRange<float> attackRange(0.001f, 2.0f, 0.001f);

  attackRange.setSkewForCentre(0.1f); // 100ms sits at the physical center

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("ENV_ATTACK", 1),

      "Env Attack",

      attackRange,

      0.01f));

  // Env Decay: 0.01s to 3.0s

  juce::NormalisableRange<float> decayRange(0.01f, 3.0f, 0.01f);

  decayRange.setSkewForCentre(0.3f);

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("ENV_DECAY", 1),

      "Env Decay",

      decayRange,

      0.1f));

  // Env Sustain: 0.0 to 1.0 (Linear amplitude scale)

  juce::NormalisableRange<float> sustainRange(0.0f, 1.0f, 0.01f);

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("ENV_SUSTAIN", 1),

      "Env Sustain",

      sustainRange,

      0.0f));

  // Env Release: 0.01s to 5.0s

  juce::NormalisableRange<float> releaseRange(0.01f, 5.0f, 0.01f);

  releaseRange.setSkewForCentre(1.0f);

  layout.add(std::make_unique<juce::AudioParameterFloat>(

      juce::ParameterID("ENV_RELEASE", 1),

      "Env Release",

      releaseRange,

      0.5f));


    // 1. Arpeggiator Rate / Subdivision Choice
    juce::StringArray rateChoices { "1/4", "1/4 Triplet", "1/8", "1/8 Triplet", "1/16", "1/16 Triplet", "1/32" };
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ARP_RATE", 1 },
        "Arp Rate",
        rateChoices,
        4 // Default index pointing to "1/16"
    ));

    // 2. Arpeggiator Gate Length Slider (from 10% to 400% to allow deep overlapping!)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_GATE", 1 },
        "Arp Gate Length",
        juce::NormalisableRange<float> (0.10f, 4.00f, 0.01f), // 10% to 400%
        0.80f // Default to 80% (standard non-overlapping)
    ));

    // 3. Arpeggiator Pattern Mode Choice
    juce::StringArray modeChoices { "Up", "Down", "Random" };
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ARP_MODE", 1 },
        "Arp Mode",
        modeChoices,
        0 // Default to "Up"
    ));

    // Inside createParameterLayout()
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_SCATTER", 1 },
        "Scatter",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), // 0% to 100%
        0.0f // Default to perfectly quantized
    ));

  return layout;
}



