#pragma once
#include <vector>
#include <cmath>
#include <juce_dsp/juce_dsp.h>

class CombFilter 
{
public:
    CombFilter() = default;
    ~CombFilter() = default;

    // Allocate memory and clear the buffer
    void prepare (const juce::dsp::ProcessSpec& spec);
    
    // Reset buffer contents to zero (prevents clicks/clicks on bypass)
    void reset();

    // Set parameters
    void setTargetFrequency(float frequencyHz);
    void setFeedback(float newFeedback); // Range: -1.0 to 1.0
    void setDamping(float newDamping);   // Range: 0.0 to 1.0 (Low-pass filter in feedback)


    // Process a single audio sample
    float processSample(float input);

private:
    // Audio engine parameters
    double mSampleRate = 44100.0;
    float mFeedback = 0.5f;
    float mDamping = 0.1f;
    float mDelayInSamples = 0.0f;
    
    // Internal Low-pass filter state for the damping loop
    float mLastLpfOutput = 0.0f;

    // DC Blocker state variables
    float mX1_loop = 0.0f; // Previous input for loop filter
    float mY1_loop = 0.0f; // Previous output for loop filter
    
    float mX1_out  = 0.0f; // Previous input for output filter
    float mY1_out  = 0.0f; // Previous output for output filter

    // Circular buffer structures
    std::vector<float> mBuffer;
    size_t mBufferSize = 0;
    size_t mWriteIndex = 0;
};