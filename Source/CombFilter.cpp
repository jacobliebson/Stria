#include "CombFilter.h"

// In CombFilter.cpp
void CombFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    mSampleRate = spec.sampleRate;
    
    // Set a maximum delay time (e.g., matching the lowest note you want, like 20 Hz)
    int maxDelaySamples = static_cast<int>(mSampleRate / 20.0f);
    
    mBufferSize = maxDelaySamples + 2;
    mBuffer.assign (mBufferSize, 0.0f);
    mWriteIndex = 0;
}

void CombFilter::reset()
{
    std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
    mLastLpfOutput = 0.0f;
    mWriteIndex = 0;
    
    // Reset DC blockers
    mX1_loop = 0.0f; mY1_loop = 0.0f;
    mX1_out  = 0.0f; mY1_out  = 0.0f;
}

void CombFilter::setTargetFrequency(float frequencyHz)
{
    // Clamp frequency to valid human hearing bounds to avoid division by zero or illegal buffer sizes
    frequencyHz = std::fmax(20.0f, std::fmin(frequencyHz, static_cast<float>(mSampleRate * 0.49)));
    
    mDelayInSamples = static_cast<float>(mSampleRate) / frequencyHz;
}

void CombFilter::setFeedback(float newFeedback)
{
    // Clamp strictly below 1.0 to guarantee stability and avoid infinite explosions
    mFeedback = std::fmax(-0.99f, std::fmin(newFeedback, 0.99f));
}

void CombFilter::setDamping(float newDamping)
{
    mDamping = std::fmax(0.0f, std::fmin(newDamping, 1.0f));
}


float CombFilter::processSample(float input)
{
    if (mBufferSize == 0) return input;

    // 1. Calculate indices and interpolate (Keep your existing working code here)
    int delayInt = static_cast<int>(mDelayInSamples);
    float fraction = mDelayInSamples - static_cast<float>(delayInt);
    int indexA = (mWriteIndex - delayInt + mBufferSize) % mBufferSize; // Safe wrap
    int indexB = (indexA + 1) % mBufferSize;
    float interpolatedDelayedSample = mBuffer[indexA] + fraction * (mBuffer[indexB] - mBuffer[indexA]);

    // 2. Apply Damping Low-Pass
    float alpha = 1.0f - mDamping;
    mLastLpfOutput = (alpha * interpolatedDelayedSample) + ((1.0f - alpha) * mLastLpfOutput);

    if (std::isnan(mLastLpfOutput) || std::isinf(mLastLpfOutput)) { mLastLpfOutput = 0.0f; }

    // 3. NEW: Apply DC Blocker inside the feedback loop
    float R = 0.995f;
    float dcBlockedFeedback = mLastLpfOutput - mX1_loop + (R * mY1_loop);
    mX1_loop = mLastLpfOutput;
    mY1_loop = dcBlockedFeedback;

    // 4. Scale feedback and saturate
    float feedbackSignal = dcBlockedFeedback * mFeedback;
    feedbackSignal = std::tanh(feedbackSignal);

    float internalLoopSignal = input + feedbackSignal;
    float currentOutput = (mFeedback == 0.0f) ? 0.0f : feedbackSignal;

    // 5. NEW: Apply a final DC Blocker to the output to clean up saturation asymmetry
    float finalOutput = currentOutput - mX1_out + (R * mY1_out);
    mX1_out = currentOutput;
    mY1_out = finalOutput;

    // 6. Write the raw combined loop signal back to the buffer
    mBuffer[mWriteIndex] = internalLoopSignal;
    mWriteIndex = (mWriteIndex + 1) % mBufferSize;

    return finalOutput;
}