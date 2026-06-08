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
    juce::AudioParameterFloatAttributes arpGainAttributes;
    arpGainAttributes = arpGainAttributes.withLabel(" dB");
    juce::NormalisableRange<float> arpGainRange(-60.0f, 24.0f, 0.1f);
    arpGainRange.setSkewForCentre(0.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ARP_GAIN", 1),
        "Arp Gain",
        arpGainRange,
        0.0f,
        arpGainAttributes
    ));

    // Chord gain
    juce::AudioParameterFloatAttributes chordGainAttributes;
    chordGainAttributes = chordGainAttributes.withLabel(" dB");
    juce::NormalisableRange<float> chordGainRange(-60.0f, 24.0f, 0.1f);
    chordGainRange.setSkewForCentre(0.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CHORD_GAIN", 1),
        "Chord Gain",
        chordGainRange,
        0.0f,
        chordGainAttributes
    ));

    // Dry wet mix
    juce::AudioParameterFloatAttributes mixAttributes;
    mixAttributes = mixAttributes.withLabel("%");
    juce::NormalisableRange<float> mixRange(0.0f, 100.0f, 0.1f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("MIX", 1),
        "Mix",
        mixRange,
        100.0f,
        mixAttributes
    ));

    
    
    // ============================================= TRIGGER ENVELOPE ============================================

    // Trigger release
    juce::AudioParameterFloatAttributes trigReleaseAttributes;
    trigReleaseAttributes = trigReleaseAttributes.withLabel(" ms");
    juce::NormalisableRange<float> trigReleaseRange(1.0f, 500.0f, 0.1f);
    trigReleaseRange.setSkewForCentre(50.0f); // 50ms center
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_RELEASE", 1),
        "Trigger Release",
        trigReleaseRange,
        100.0f,
        trigReleaseAttributes
    ));

    // Trigger softness
    juce::AudioParameterFloatAttributes trigAttackAttributes;
    trigAttackAttributes = trigAttackAttributes.withLabel(" ms");
    juce::NormalisableRange<float> trigSoftnessRange(0.0f, 200.0f, 0.1f);
    trigSoftnessRange.setSkewForCentre(25.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_SOFTNESS", 1),
        "Trigger attack",
        trigSoftnessRange,
        0.0f,
        trigAttackAttributes
    ));

    // Trigger threshold
    juce::AudioParameterFloatAttributes trigThreshAttributes;
    trigThreshAttributes = trigThreshAttributes.withLabel(" dB");
    juce::NormalisableRange<float> trigThreshRange(0.0f, 24.0f, 0.1f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_THRESHOLD", 1),
        "Trigger threshold",
        trigThreshRange,
        0.0f,
        trigThreshAttributes
    ));

    
    
    // ============================================= SYNTH ENVELOPE =============================================

    // Voice attack
    juce::AudioParameterFloatAttributes attackAttributes;
    attackAttributes = attackAttributes.withLabel(" s");
    juce::NormalisableRange<float> attackRange(0.001f, 2.0f, 0.001f);
    attackRange.setSkewForCentre(0.1f); // 100ms sits at the physical center
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV_ATTACK", 1),
        "Env Attack",
        attackRange,
        0.01f,
        attackAttributes
    ));

    // Voice decay
    juce::AudioParameterFloatAttributes decayAttributes;
    decayAttributes = decayAttributes.withLabel(" s");
    juce::NormalisableRange<float> decayRange(0.01f, 3.0f, 0.01f);
    decayRange.setSkewForCentre(0.3f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV_DECAY", 1),
        "Env Decay",
        decayRange,
        0.1f,
        decayAttributes
    ));

    // Voice sustain
    juce::AudioParameterFloatAttributes sustainAttributes;
    sustainAttributes = sustainAttributes.withLabel("%");
    juce::NormalisableRange<float> sustainRange(0.0f, 1.0f, 0.01f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV_SUSTAIN", 1),
        "Env Sustain",
        sustainRange,
        0.0f,
        sustainAttributes
    ));

    // Voice release
    juce::AudioParameterFloatAttributes releaseAttributes;
    releaseAttributes = releaseAttributes.withLabel(" s");
    juce::NormalisableRange<float> releaseRange(0.01f, 5.0f, 0.01f);
    releaseRange.setSkewForCentre(1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ENV_RELEASE", 1),
        "Env Release",
        releaseRange,
        0.5f,
        releaseAttributes
    ));

    
    
    // ============================================= ARP CONTROLS =============================================

    // 1. Create and configure the attributes object for the subdivision rates
        juce::AudioParameterIntAttributes rateAttributes;

        rateAttributes = rateAttributes.withStringFromValueFunction ([](float value, int maxLen) -> juce::String {
            switch (static_cast<int> (value))
            {
                case 0:  return "1/4";
                case 1:  return "1/4 T";
                case 2:  return "1/8";
                case 3:  return "1/8 T";
                case 4:  return "1/16";
                case 5:  return "1/16 T";
                case 6:  return "1/32";
                default: return "1/16";
            }
        });

        rateAttributes = rateAttributes.withValueFromStringFunction ([](const juce::String& text) -> float {
            if (text.equalsIgnoreCase ("1/4"))    return 0.0f;
            if (text.equalsIgnoreCase ("1/4 T"))  return 1.0f;
            if (text.equalsIgnoreCase ("1/8"))    return 2.0f;
            if (text.equalsIgnoreCase ("1/8 T"))  return 3.0f;
            if (text.equalsIgnoreCase ("1/16"))   return 4.0f;
            if (text.equalsIgnoreCase ("1/16 T")) return 5.0f;
            if (text.equalsIgnoreCase ("1/32"))   return 6.0f;
            return 4.0f; // fallback to 1/16
        });

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { "ARP_RATE", 1 },
            "Arp Rate",
            0, 6, 4,  // min, max, default (1/16)
            rateAttributes
        ));
    
    // Arpeggiator gate length 
    juce::AudioParameterFloatAttributes arpGateAttributes;
    arpGateAttributes = arpGateAttributes.withLabel("%");
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_GATE", 1 },
        "Arp Gate Length",
        juce::NormalisableRange<float> (10.00f, 400.0f, 0.1f), // 10% to 400%
        100.0f,
        arpGateAttributes
    ));

    // Arpeggiator mode
    // When adding new mode, update enum class in Arpeggiator.h in exact order

        // Create and configure the attributes object
        juce::AudioParameterIntAttributes modeAttributes;
        
        modeAttributes = modeAttributes.withStringFromValueFunction ([](float value, int maxLen) -> juce::String {
            switch (static_cast<int> (value))
            {
                case 0:  return "Up";
                case 1:  return "Down";
                case 2:  return "Up/down";
                default: return "Unknown";
            }
        });

        modeAttributes = modeAttributes.withValueFromStringFunction ([](const juce::String& text) -> float {
            if (text.equalsIgnoreCase ("Up"))      return 0.0f;
            if (text.equalsIgnoreCase ("Down"))    return 1.0f;
            if (text.equalsIgnoreCase ("Up/down"))  return 2.0f;
            return 0.0f;
        });

        // Pass the attributes object as the final argument to AudioParameterInt
        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { "ARP_MODE", 1 },
            "Arp Mode",
            0, 2, 0,  // min, max, default
            modeAttributes
        ));

    // Arpeggiatior deviation - chance to reandomly swap a note
    juce::AudioParameterFloatAttributes arpDevAttributes;
    arpDevAttributes = arpDevAttributes.withLabel("%");
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_DEVIATION", 1 },
        "Deviation",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), // 0% to 100%
        0.0f,
        arpDevAttributes
    ));

    // Arpeggiator scatter - random time offset
    juce::AudioParameterFloatAttributes arpScatAttributes;
    arpScatAttributes = arpScatAttributes.withLabel("%");
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_SCATTER", 1 },
        "Scatter",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), // 0% to 100%
        0.0f,
        arpScatAttributes
    ));

    // Arpeggiator octave range
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_RANGE", 1 },
        "Octave Range",
        juce::NormalisableRange<float> (-4.0f, 4.0f, 1.0f), 
        0.0f
    ));

    return layout;
}



