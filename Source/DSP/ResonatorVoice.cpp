// Source/ResonatorVoice.cpp
#include "ResonatorVoice.h"


ResonatorVoice::ResonatorVoice(HaltonGenerator& generator) : haltonPanner(generator)
{
    // 1. Initialize your custom parameters mapping layout
    CustomADSR::Parameters params;
    params.attack  = 0.01f; 
    params.decay   = 0.1f;
    params.sustain = 1.0f;
    params.release = 1.5f;
    params.hold = 0.0f;
    params.useHoldPhase = false;  
    adsr.setParameters (params);
    logger = std::make_unique<DebugLogger>(juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("voice_debug_log.txt").getFullPathName());
}

float ResonatorVoice::getEnvelopeLevel() 
{ 
    return adsr.getEnvLevel(); 
}

float ResonatorVoice::getCurrentFrequency() 
{ 
    return currentFrequency; 
}

bool ResonatorVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<ResonatorSound*> (sound) != nullptr;
}

void ResonatorVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition)
{
    juce::ignoreUnused(velocity, sound, currentPitchWheelPosition);

    // 1. Cache the base MIDI note as a float
    float baseMidi = static_cast<float>(midiNoteNumber);
    float processedMidi = baseMidi;

    // 2. Apply detune offset in SEMITONE space
    if (detuneMode == 0) // Note Mode
    {
        // Random value between -1.0 and 1.0
        float randomRange = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        processedMidi += randomRange * detuneAmount;
    }

    // calculate pan position
    currentPan = haltonPanner.getNextValue();

    // 3. Convert to frequency
    baseFrequency = 440.0f * std::pow(2.0f, (baseMidi - 69.0f) / 12.0f);
    float processedFreq = 440.0f * std::pow(2.0f, (processedMidi - 69.0f) / 12.0f);
    currentFrequency = processedFreq;

    leftFilter.setTargetFrequency(processedFreq);
    rightFilter.setTargetFrequency(processedFreq);

    leftFilter.reset();
    rightFilter.reset();
    
    adsr.noteOn();
}

void ResonatorVoice::stopNote (float velocity, bool allowTailOff)
{
    juce::ignoreUnused(velocity);

    if (sustainStatePtr && sustainStatePtr->load())
    {
        isSustainHeld = true; 
        return; 
    }

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

void ResonatorVoice::pitchWheelMoved (int newPitchWheelValue) {
    juce::ignoreUnused(newPitchWheelValue);
}
void ResonatorVoice::controllerMoved (int controllerNumber, int newControllerValue) {
    juce::ignoreUnused(controllerNumber);
    juce::ignoreUnused(newControllerValue);
}

void ResonatorVoice::prepare (const juce::dsp::ProcessSpec& spec)
{
    leftFilter.prepare (spec);
    rightFilter.prepare (spec);
    adsr.setSampleRate (spec.sampleRate);
}

// 2. Change the incoming argument to match your custom parameter struct definition
void ResonatorVoice::updateParameters (float feedback, float damping, const CustomADSR::Parameters& envParams, float detune, int mode)
{
    leftFilter.setFeedback (feedback);
    rightFilter.setFeedback (feedback);
    leftFilter.setDamping (damping);
    rightFilter.setDamping (damping);
    
    adsr.setParameters (envParams);

    detuneAmount = detune;
    detuneMode = mode;
}

void ResonatorVoice::processExcitation (float inputL, float inputR, float& outputL, float& outputR)
{
    if (isSustainHeld && sustainStatePtr && !sustainStatePtr->load())
    {
        adsr.noteOff();
        isSustainHeld = false;
    }
    // Update drift/shake logic if mode is 1 (Drift) or 2 (shake)
    if (detuneMode != 0) 
    {
        // 1 = Drift (Slow), 2 = shake (Fast)
        int threshold = (detuneMode == 1) ? 600 : 100;

        if (++driftCounter >= threshold) 
        {
            float shake = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * detuneAmount;
            targetDetuneOffset = (targetDetuneOffset * 0.95f) + shake;
            targetDetuneOffset = juce::jlimit (-detuneAmount, detuneAmount, targetDetuneOffset);
            driftCounter = 0;
        }

        float smoothing = (detuneMode == 1) ? 0.999f : 0.99f;
        
        // Linear interpolation for smooth pitch movement
        currentDetuneOffset = currentDetuneOffset * smoothing + targetDetuneOffset * (1.0f - smoothing);
        
        float freq = baseFrequency * std::pow(2.0f, currentDetuneOffset / 12.0f);
        currentFrequency = freq;
        leftFilter.setTargetFrequency(freq);
        rightFilter.setTargetFrequency(freq);
    }

    // Standard audio processing
    float envelopeGain = adsr.getNextSample();

    float leftGain  = std::cos (currentPan * juce::MathConstants<float>::halfPi);
    float rightGain = std::sin (currentPan * juce::MathConstants<float>::halfPi);

    float wetL = leftFilter.processSample (inputL * envelopeGain) * leftGain;
    float wetR = rightFilter.processSample (inputR * envelopeGain) * rightGain;

    outputL += wetL;
    outputR += wetR;

    if (!adsr.isActive())
    {
        if (std::abs (wetL) < 1e-6f && std::abs (wetR) < 1e-6f)
            clearCurrentNote();
    }
}

void ResonatorVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    juce::ignoreUnused(outputBuffer);
    juce::ignoreUnused(startSample);
    juce::ignoreUnused(numSamples);
}

void ResonatorVoice::forceStop()
{
    isSustainHeld = false;
    adsr.reset();
    leftFilter.reset();
    rightFilter.reset();
    clearCurrentNote();
}