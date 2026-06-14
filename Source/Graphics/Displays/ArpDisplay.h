// Source/ArpDisplay.h
#pragma once

#include "../ResonatorPalette.h"

#include <juce_audio_processors/juce_audio_processors.h>



// A read-only arpeggiator pattern visualizer.
// Shows relative note heights, gate length, and scatter as a row of bars.
class ArpDisplay : public juce::Component,
                   private juce::AudioProcessorValueTreeState::Listener
{
public:
    ArpDisplay (juce::AudioProcessorValueTreeState& apvts,
                const juce::String& rateId,
                const juce::String& gateId,
                const juce::String& modeId,
                const juce::String& scatterId,
                const juce::String& octaveRangeId);

    ~ArpDisplay() override;

    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    void parameterChanged (const juce::String& paramId, float newValue) override;

    // Returns a set of relative note heights [0,1] for the current mode
    std::vector<float> buildNotePattern() const;

    juce::AudioProcessorValueTreeState& apvts;

    juce::String rateId, gateId, modeId, scatterId, octaveRangeId;

    float gate        = 0.8f;
    float scatter     = 0.0f;
    int   mode        = 0;
    int   octaveRange = 0;

    // Number of steps to display
    static constexpr int numDisplaySteps = 8;
};
