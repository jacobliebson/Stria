// Source/EnvelopeDisplay.h
#pragma once

#include "../ResonatorPalette.h"

#include <juce_audio_processors/juce_audio_processors.h>




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

    // Set the accent colour (call before or after setParameters)
    void setAccentColour (juce::Colour colour) { accentColour = colour; repaint(); }

    // Swap the parameter set this display listens to
    void setParameters (const juce::String& newAttackId,
                        const juce::String& newDecayId,
                        const juce::String& newSustainId,
                        const juce::String& newReleaseId);

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
    juce::Colour accentColour = ResonatorPalette::accentPrimary(); // defaults to purple

    static constexpr float maxAttack  = 2.0f;
    static constexpr float maxDecay   = 2.0f;
    static constexpr float maxRelease = 4.0f;
};
