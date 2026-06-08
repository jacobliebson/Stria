// Source/EnvelopeDisplay.h
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ResonatorPalette.h"

// A read-only ADSR curve visualizer driven by APVTS parameter values.
class EnvelopeDisplay : public juce::Component,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    EnvelopeDisplay (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& attackId,
                     const juce::String& decayId,
                     const juce::String& sustainId,
                     const juce::String& releaseId);

    ~EnvelopeDisplay() override;

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    void parameterChanged (const juce::String& paramId, float newValue) override;
    juce::Path buildCurve (juce::Rectangle<float> bounds) const;

    juce::AudioProcessorValueTreeState& apvts;

    juce::String attackId, decayId, sustainId, releaseId;

    float attack  = 0.01f;
    float decay   = 0.1f;
    float sustain = 1.0f;
    float release = 0.5f;

    // Max times in seconds used to normalise the display
    static constexpr float maxAttack  = 2.0f;
    static constexpr float maxDecay   = 2.0f;
    static constexpr float maxRelease = 4.0f;
};
