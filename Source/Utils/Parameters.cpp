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

    // Resonator max detune in semitones
    juce::NormalisableRange<float> detuneRange(0.0f, 1.0f, 0.01f, 1.0f);
    detuneRange.setSkewForCentre(0.1f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("DETUNE", 1),
        "Detune",
        detuneRange,
        0.0f
    ));

    // Resonator detune mode

        juce::AudioParameterIntAttributes detuneModeAttributes;
        detuneModeAttributes = detuneModeAttributes.withStringFromValueFunction([](float value, int maxLen) -> juce::String {
            switch (static_cast<int> (value))
            {
                case 0:  return "Note";
                case 1:  return "Drift";
                case 2:  return "Shake";
                default: return "Unknown";
            }
        });

        layout.add (std::make_unique<juce::AudioParameterInt> (
            juce::ParameterID { "DETUNE_MODE", 1 },
            "Detune Mode",
            0, 2, 0,  // min, max, default
            detuneModeAttributes
        ));

    
    // ============================================= OUTPUT BALANCE =============================================

    // Arp gain
    juce::AudioParameterFloatAttributes arpGainAttributes;
    arpGainAttributes = arpGainAttributes.withLabel(" dB");
    juce::NormalisableRange<float> arpGainRange(-60.0f, 24.0f, 0.01f);
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
    juce::NormalisableRange<float> chordGainRange(-60.0f, 24.0f, 0.01f);
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
    juce::NormalisableRange<float> mixRange(0.0f, 100.0f, 1.0f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("MIX", 1),
        "Mix",
        mixRange,
        100.0f,
        mixAttributes
    ));

    // Stereo spread
    juce::AudioParameterFloatAttributes spreadAttributes;
    spreadAttributes = spreadAttributes.withLabel("%");
    juce::NormalisableRange<float> spreadRange(0.0f, 100.0f, 1.0f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("SPREAD", 1),
        "Spread",
        spreadRange,
        0.0f,
        spreadAttributes
    ));

    // Chord Pan
    juce::AudioParameterFloatAttributes chordPanAttributes;
    chordPanAttributes = chordPanAttributes.withLabel("%");
    juce::NormalisableRange<float> chordPanRange(-100.0f, 100.0f, 1.0f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CHORD_PAN", 1),
        "Pan",
        chordPanRange,
        0.0f,
        chordPanAttributes
    ));

    // Arp Pan
    juce::AudioParameterFloatAttributes arpPanAttributes;
    arpPanAttributes = arpPanAttributes.withLabel("%");
    juce::NormalisableRange<float> arpPanRange(-100.0f, 100.0f, 1.0f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ARP_PAN", 1),
        "Pan",
        arpPanRange,
        0.0f,
        arpPanAttributes
    ));

    
    
    // ============================================= TRIGGER ENVELOPE ============================================

    // Trigger attack
    juce::AudioParameterFloatAttributes trigAttackAttributes;
    trigAttackAttributes = trigAttackAttributes.withLabel(" ms");
    juce::NormalisableRange<float> trigAttackRange(0.0f, 500.0f, 0.1f);
    trigAttackRange.setSkewForCentre(50.0f); // 50ms center
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_ATTACK", 1),
        "Trigger Attack",
        trigAttackRange,
        0.0f,
        trigAttackAttributes
    ));

    // Trigger release
    juce::AudioParameterFloatAttributes trigReleaseAttributes;
    trigReleaseAttributes = trigReleaseAttributes.withLabel(" ms");
    juce::NormalisableRange<float> trigReleaseRange(0.0f, 500.0f, 0.1f);
    trigReleaseRange.setSkewForCentre(50.0f); // 50ms center
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_RELEASE", 1),
        "Trigger Release",
        trigReleaseRange,
        1.0f,
        trigReleaseAttributes
    ));

    // Trigger hold
    juce::AudioParameterFloatAttributes trigHoldAttributes;
    trigHoldAttributes = trigHoldAttributes.withLabel(" ms");
    juce::NormalisableRange<float> trigHoldRange(0.0f, 500.0f, 0.1f);
    trigHoldRange.setSkewForCentre(50.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_HOLD", 1),
        "Trigger Hold",
        trigHoldRange,
        0.02f,
        trigHoldAttributes
    ));

    // Trigger threshold
    juce::AudioParameterFloatAttributes trigThreshAttributes;
    trigThreshAttributes = trigThreshAttributes.withLabel(" dB");
    juce::NormalisableRange<float> trigThreshRange(-60.0f, 0.0f, 0.1f);
    trigThreshRange.setSkewForCentre(-12.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("TRIG_THRESHOLD", 1),
        "Trigger threshold",
        trigThreshRange,
        -27.0f,
        trigThreshAttributes
    ));

    // Legato mode toggle
    juce::AudioParameterBoolAttributes triggerModeAttributes;
    triggerModeAttributes = triggerModeAttributes.withStringFromValueFunction([](bool value, int maxLen) -> juce::String {
        if (value) 
            return "Legato";
        else 
            return "Retrigger";
    });

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("TRIG_MODE", 1),
        "Mode",
        true,
        triggerModeAttributes
    ));


    
    
    // ============================================= CHORD ENVELOPE =============================================

    // Chord voice attack
    juce::AudioParameterFloatAttributes chordAttackAttributes;
    chordAttackAttributes = chordAttackAttributes.withLabel(" s");
    juce::NormalisableRange<float> chordAttackRange(0.0f, 2.0f, 0.001f);
    chordAttackRange.setSkewForCentre(0.2f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CHORD_ENV_ATTACK", 1),
        "Chord Env Attack",
        chordAttackRange,
        0.5f,
        chordAttackAttributes
    ));

    // Chord voice decay
    juce::AudioParameterFloatAttributes chordDecayAttributes;
    chordDecayAttributes = chordDecayAttributes.withLabel(" s");
    juce::NormalisableRange<float> chordDecayRange(0.0f, 3.0f, 0.01f);
    chordDecayRange.setSkewForCentre(0.3f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CHORD_ENV_DECAY", 1),
        "Chord Env Decay",
        chordDecayRange,
        0.3f,
        chordDecayAttributes
    ));

    // Chord voice sustain
    juce::AudioParameterFloatAttributes chordSustainAttributes;
    chordSustainAttributes = chordSustainAttributes.withLabel("%");
    juce::NormalisableRange<float> chordSustainRange(0.0f, 100.0f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CHORD_ENV_SUSTAIN", 1),
        "Chord Env Sustain",
        chordSustainRange,
        100.0f,
        chordSustainAttributes
    ));

    // Chord voice release
    juce::AudioParameterFloatAttributes chordReleaseAttributes;
    chordReleaseAttributes = chordReleaseAttributes.withLabel(" s");
    juce::NormalisableRange<float> chordReleaseRange(0.0f, 5.0f, 0.01f);
    chordReleaseRange.setSkewForCentre(1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("CHORD_ENV_RELEASE", 1),
        "Chord Env Release",
        chordReleaseRange,
        1.0f,
        chordReleaseAttributes
    ));

    // ============================================= ARP ENVELOPE =============================================

    // Arp voice attack
    juce::AudioParameterFloatAttributes arpAttackAttributes;
    arpAttackAttributes = arpAttackAttributes.withLabel(" s");
    juce::NormalisableRange<float> arpAttackRange(0.0f, 2.0f, 0.001f);
    arpAttackRange.setSkewForCentre(0.1f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ARP_ENV_ATTACK", 1),
        "Arp Env Attack",
        arpAttackRange,
        0.0f,
        arpAttackAttributes
    ));

    // Arp voice decay
    juce::AudioParameterFloatAttributes arpDecayAttributes;
    arpDecayAttributes = arpDecayAttributes.withLabel(" s");
    juce::NormalisableRange<float> arpDecayRange(0.0f, 3.0f, 0.01f);
    arpDecayRange.setSkewForCentre(0.3f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ARP_ENV_DECAY", 1),
        "Arp Env Decay",
        arpDecayRange,
        0.2f,
        arpDecayAttributes
    ));

    // Arp voice sustain
    juce::AudioParameterFloatAttributes arpSustainAttributes;
    arpSustainAttributes = arpSustainAttributes.withLabel("%");
    juce::NormalisableRange<float> arpSustainRange(0.0f, 100.0f, 1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ARP_ENV_SUSTAIN", 1),
        "Arp Env Sustain",
        arpSustainRange,
        0.0f,
        arpSustainAttributes
    ));

    // Arp voice release
    juce::AudioParameterFloatAttributes arpReleaseAttributes;
    arpReleaseAttributes = arpReleaseAttributes.withLabel(" s");
    juce::NormalisableRange<float> arpReleaseRange(0.0f, 5.0f, 0.01f);
    arpReleaseRange.setSkewForCentre(1.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ARP_ENV_RELEASE", 1),
        "Arp Env Release",
        arpReleaseRange,
        0.5f,
        arpReleaseAttributes
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
        juce::NormalisableRange<float> (10.00f, 400.0f, 1.0f), // 10% to 400%
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
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), // 0% to 100%
        0.0f,
        arpDevAttributes
    ));

    // Arpeggiator scatter - random time offset
    juce::AudioParameterFloatAttributes arpScatAttributes;
    arpScatAttributes = arpScatAttributes.withLabel("%");
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ARP_SCATTER", 1 },
        "Scatter",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), // 0% to 100%
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