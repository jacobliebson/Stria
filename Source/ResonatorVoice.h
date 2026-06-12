#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "CombFilter.h"
#include "CustomADSR.h"
#include "HaltonGenerator.h"

// JUCE Synthesizer requires a sound definition to match voices against
class ResonatorSound : public juce::SynthesiserSound
{
public:
    // Returns true if this sound should play when a given MIDI note is pressed
    bool appliesToNote (int midiNoteNumber) override { juce::ignoreUnused(midiNoteNumber); return true; }
    
    // Returns true if the sound should be triggered by MIDI events on this channel
    bool appliesToChannel (int midiChannel) override { juce::ignoreUnused(midiChannel); return true; }
};

class ResonatorVoice : public juce::SynthesiserVoice
{
public:
    ResonatorVoice(HaltonGenerator& generator);
    ~ResonatorVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void setSustainStatePointer(std::atomic<bool>* ptr) { sustainStatePtr = ptr; }
    
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void prepare (const juce::dsp::ProcessSpec& spec);
    // In Source/ResonatorVoice.h
    void updateParameters (float feedback, float damping, const CustomADSR::Parameters& envParams, float detune, int mode);
    void processExcitation (float inputL, float inputR, float& outputL, float& outputR);

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    // Returns the current envelope level without consuming a sample.
    float getEnvelopeLevel();

    float getCurrentFrequency();

    void setHaltonGenerator(HaltonGenerator& generator);
 
    // Returns the MIDI note this voice was started on (0 if inactive)
    int getCurrentMidiNote() const { return isVoiceActive() ? getCurrentlyPlayingNote() : -1; }

private:
    CombFilter leftFilter;
    CombFilter rightFilter;
    CustomADSR adsr;

    std::atomic<bool>* sustainStatePtr = nullptr;
    bool isSustainHeld; // if note is being held due to sustain

    float baseFrequency;
    float currentFrequency;

    int detuneMode;
    float detuneAmount;
    int driftCounter;
    float currentDetuneOffset;
    float targetDetuneOffset;

    HaltonGenerator& haltonPanner;
    float currentPan = 0.5f;
};