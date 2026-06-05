#include "ResonatorVoice.h"

ResonatorVoice::ResonatorVoice()
{
    juce::ADSR::Parameters params;
    params.attack  = 0.01f; // Fast transient response
    params.decay   = 0.1f;
    params.sustain = 1.0f;
    params.release = 1.5f;  // Long resonant tail
    adsr.setParameters (params);
}

bool ResonatorVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<ResonatorSound*> (sound) != nullptr;
}

void ResonatorVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    float freq = 440.0f * std::pow (2.0f, (static_cast<float> (midiNoteNumber) - 69.0f) / 12.0f);
    
    leftFilter.setTargetFrequency (freq);
    rightFilter.setTargetFrequency (freq);
    
    adsr.noteOn();
}

void ResonatorVoice::stopNote (float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
    }
}

void ResonatorVoice::pitchWheelMoved (int newPitchWheelValue) {}
void ResonatorVoice::controllerMoved (int controllerNumber, int newControllerValue) {}

void ResonatorVoice::prepare (const juce::dsp::ProcessSpec& spec)
{
    leftFilter.prepare (spec);
    rightFilter.prepare (spec);
    adsr.setSampleRate (spec.sampleRate);
}

// In Source/ResonatorVoice.cpp
void ResonatorVoice::updateParameters (float feedback, float damping, const juce::ADSR::Parameters& envParams)
{
    leftFilter.setFeedback (feedback);
    rightFilter.setFeedback (feedback);
    leftFilter.setDamping (damping);
    rightFilter.setDamping (damping);
    
    // Smoothly sets the updated envelope targets
    adsr.setParameters (envParams);
}

void ResonatorVoice::processExcitation (float inputL, float inputR, float& outputL, float& outputR)
{
    if (!adsr.isActive())
    {
        clearCurrentNote();
        return;
    }

    float envelopeGain = adsr.getNextSample();

    float wetL = leftFilter.processSample (inputL * envelopeGain);
    float wetR = rightFilter.processSample (inputR * envelopeGain);

    outputL += wetL * envelopeGain;
    outputR += wetR * envelopeGain;
}

void ResonatorVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // Left blank intentionally because we use custom sample-accurate 
    // excitation streaming via processExcitation()
}