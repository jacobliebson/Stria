// Source/CustomADSR.cpp
#include "CustomADSR.h"

CustomADSR::CustomADSR() noexcept {}

void CustomADSR::setParameters (const Parameters& newParameters) noexcept
{
    parameters = newParameters;
    recalculateRates();
}

void CustomADSR::setSampleRate (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    recalculateRates();
}

void CustomADSR::reset() noexcept
{
    state = State::idle;
    envelopeVal = 0.0f;
}

float CustomADSR::getEnvLevel() 
{
    return envelopeVal;
}

void CustomADSR::noteOn() noexcept
{

    if (attackRate > 0.0f)
    {
        state = State::attack;
    }
    else if (decayRate > 0.0f)
    {
        envelopeVal = 1.0f;
        state = State::decay;
    }
    else
    {
        envelopeVal = parameters.sustain;
        state = State::sustain;
    }
}

void CustomADSR::noteOff() noexcept
{
    if (state != State::idle)
    {
        if (parameters.release > 0.0f)
        {
            releaseRate = (float) (envelopeVal / (parameters.release * sampleRate));
            state = State::release;
        }
        else
        {
            reset();
        }
    }
}

float CustomADSR::getNextSample() noexcept
{
    switch (state)
    {
        case State::idle:
        {
            return 0.0f;
        }

        case State::attack:
        {
            envelopeVal += attackRate;

            if (envelopeVal >= 1.0f)
            {
                envelopeVal = 1.0f;
                goToNextState();
            }

            break;
        }

        case State::decay:
        {
            envelopeVal -= decayRate;

            if (envelopeVal <= parameters.sustain)
            {
                envelopeVal = parameters.sustain;
                goToNextState();
            }

            break;
        }

        case State::sustain:
        {
            envelopeVal = parameters.sustain;
            break;
        }

        case State::release:
        {
            envelopeVal -= releaseRate;

            if (envelopeVal <= 0.0f)
            {
                envelopeVal = 0.0f; // clamp before returning
                goToNextState();
            }

            break;
        }
    }

    return envelopeVal;
}



void CustomADSR::goToNextState() noexcept
{
    if (state == State::attack)
    {
        state = (decayRate > 0.0f ? State::decay : State::sustain);
        return;
    }

    if (state == State::decay)
    {
        state = State::sustain;
        return;
    }

    if (state == State::release)
        reset();
}

void CustomADSR::recalculateRates() noexcept
{
    auto getRate = [] (float distance, float timeInSeconds, double sr)
    {
        return timeInSeconds > 0.0f ? (float) (distance / (timeInSeconds * sr)) : -1.0f;
    };

    attackRate = getRate (1.0f, parameters.attack, sampleRate);
    decayRate  = getRate (1.0f - parameters.sustain, parameters.decay, sampleRate);
    // releaseRate is intentionally omitted — it's calculated dynamically in noteOff()
    // based on envelopeVal at that moment, so it correctly covers only the remaining distance.

    if ((state == State::attack && attackRate <= 0.0f)
        || (state == State::decay && (decayRate <= 0.0f || envelopeVal <= parameters.sustain))
        || (state == State::release && releaseRate <= 0.0f))
    {
        goToNextState();
    }
}