#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "CombFilter.h"
#include "CustomADSR.h"

// JUCE Synthesizer requires a sound definition to match voices against
class ResonatorSound : public juce::SynthesiserSound
{
public:
    // Returns true if this sound should play when a given MIDI note is pressed
    bool appliesToNote (int midiNoteNumber) override { return true; }
    
    // Returns true if the sound should be triggered by MIDI events on this channel
    bool appliesToChannel (int midiChannel) override { return true; }
};

class ResonatorVoice : public juce::SynthesiserVoice
{
public:
    ResonatorVoice();
    ~ResonatorVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void prepare (const juce::dsp::ProcessSpec& spec);
    // In Source/ResonatorVoice.h
    void updateParameters (float feedback, float damping, const CustomADSR::Parameters& envParams);
    void processExcitation (float inputL, float inputR, float& outputL, float& outputR);

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    CombFilter leftFilter;
    CombFilter rightFilter;
    CustomADSR adsr;
};