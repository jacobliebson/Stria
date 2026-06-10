#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class CustomADSR
{
public:
    struct Parameters
    {
        float attack = 0.1f, decay = 0.1f, sustain = 1.0f, release = 0.1f, hold = 0.5f;
        bool useHoldPhase = false;
    };

    CustomADSR() noexcept;
    ~CustomADSR() = default;

    void setParameters (const Parameters& newParameters) noexcept;
    const Parameters& getParameters() const noexcept { return parameters; }

    bool isActive() const noexcept { return state != State::idle; }

    void setSampleRate (double newSampleRate) noexcept;

    void reset() noexcept;
    void noteOn() noexcept;

    void noteOff() noexcept;

    float getNextSample() noexcept;

    float getEnvLevel() noexcept;

    // Add to public section
    int getStateInt() const noexcept { return static_cast<int> (state); }
    int32_t getHoldCounter() const noexcept { return holdCounter; }
    


private:
    enum class State { idle, attack, decay, hold, sustain, release };

    void recalculateRates() noexcept;
    void goToNextState() noexcept;

    State state = State::idle;
    Parameters parameters;
    double sampleRate = 44100.0;
    float envelopeVal = 0.0f;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;
   
    int32_t holdCounter = 0;
    int32_t holdSamples = 0;
};