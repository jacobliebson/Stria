#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class CustomADSR
{
public:
    struct Parameters
    {
        float attack = 0.1f, decay = 0.1f, sustain = 1.0f, release = 0.1f;
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
    


private:
    enum class State { idle, attack, decay, sustain, release };

    void recalculateRates() noexcept;
    void goToNextState() noexcept;

    State state = State::idle;
    Parameters parameters;
    double sampleRate = 44100.0;
    float envelopeVal = 0.0f;
    float attackRate = 0.0f, decayRate = 0.0f, releaseRate = 0.0f;
};