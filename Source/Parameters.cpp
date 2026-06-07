#include "Parameters.h"

juce::AudioProcessorValueTreeState::ParameterLayout Parameters::configureParameters () {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::NormalisableRange<float> feedbackRange(-1.0f, 1.0f, 0.01f, 1.0f);

    
    
    // ============================================= RESONATOR =============================================

    // Resonator feedback
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FEEDBACK", 1),
        "Feedback",
        feedbackRange,
        0.8f
    ));

    // Resonator damping
    juce::NormalisableRange<float> dampingRange(0.0f, 1.0f, 0.01f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("DAMPING", 1),
        "Damping",
        dampingRange,
        0.5f
    ));

    
    
    // ============================================= OUTPUT BALANCE =============================================

    // Arp gain
    juce::NormalisableRange<float> arpGainRange(-60.0f, 24.0f, 0.1f);
    arpGainRange.setSkewForCentre(0.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ARP_GAIN", 1),
        "Arp Gain",
        arpGainRange,
        6.0f
    ));

    // Chord gain
    juce::NormalisableRange<float> chordGainRange(-60.0f, 24.0f, 0.1f);
    chordGainRange.setSkewForCentre(0.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CHORD_GAIN", 1),
        "Chord Gain",
        chordGainRange,
        6.0f
    ));

    // Dry wet mix
    juce::NormalisableRange<float> mixRange(0.0f, 100.0f, 0.1f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("MIX", 1),
        "Mix",
        mixRange,
        100.0f
    ));

    
    
    // ============================================= TRIGGER ENVELOPE ============================================

    // Trigger release
    juce::NormalisableRange<float> trigReleaseRange(1.0f, 500.0f, 0.1f);
    trigReleaseRange.setSkewForCentre(50.0f); // 50ms center
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_RELEASE", 1),
        "Trigger Release",
        trigReleaseRange,
        100.0f
    ));

    // Trigger softness
    juce::NormalisableRange<float> trigSoftnessRange(0.0f, 200.0f, 0.1f);
    trigSoftnessRange.setSkewForCentre(25.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_SOFTNESS", 1),
        "Softness",
        trigSoftnessRange,
        0.0f
    ));

    // Trigger threshold
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_THRESHOLD", 1),
        "Trigger threshold",
        0.0f,
        24.0f,
        6.0f
    ));

    
    
    // ============================================= SYNTH ENVELOPE =============================================

    // Voice attack
    juce::NormalisableRange<float> attackRange(0.001f, 2.0f, 0.001f);
    attackRange.setSkewForCentre(0.1f); // 100ms sits at the physical center
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV_ATTACK", 1),
        "Env Attack",
        attackRange,
        0.01f
    ));

    // Voice decay
    juce::NormalisableRange<float> decayRange(0.01f, 3.0f, 0.01f);
    decayRange.setSkewForCentre(0.3f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV_DECAY", 1),
        "Env Decay",
        decayRange,
        0.1f
    ));

    // Voice sustain
    juce::NormalisableRange<float> sustainRange(0.0f, 1.0f, 0.01f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV_SUSTAIN", 1),
        "Env Sustain",
        sustainRange,
        0.0f
    ));

    // Voice release
    juce::NormalisableRange<float> releaseRange(0.01f, 5.0f, 0.01f);
    releaseRange.setSkewForCentre(1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV_RELEASE", 1),
        "Env Release",
        releaseRange,
        0.5f
    ));

    
    
    // ============================================= ARP CONTROLS =============================================

    // Arpeggiator subdivision
    juce::StringArray rateChoices { "1/4", "1/4 Triplet", "1/8", "1/8 Triplet", "1/16", "1/16 Triplet", "1/32" };
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ARP_RATE", 1 },
        "Arp Rate",
        rateChoices,
        4 
    ));

    // Arpeggiator gate length 
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_GATE", 1 },
        "Arp Gate Length",
        juce::NormalisableRange<float> (0.10f, 4.00f, 0.01f), // 10% to 400%
        0.80f
    ));

    // Arpeggiator mode
    // When adding new mode, update enum class in Arpeggiator.h in exact order
    juce::StringArray modeChoices { "Up", "Down", "Up-down", "Random" };
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "ARP_MODE", 1 },
        "Arp Mode",
        modeChoices,
        0 
    ));

    // Arpeggiator scatter
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_SCATTER", 1 },
        "Scatter",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), // 0% to 100%
        0.0f
    ));

    return layout;
}



