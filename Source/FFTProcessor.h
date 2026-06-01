#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <complex>

class FFTProcessor
{
public:
    void orderChanged (int newFFTOrder, double newSampleRate);
    void setCutoffFrequency (float frequencyInHz);
    void setDeltaMode (bool shouldBeDelta);
    
    void pushSample (float sample);
    float popSample();

    void processFrequencyDomain();

private:
    bool isDeltaModeActive = false;
    int fftOrder = 11; 
    int fftSize  = 2048;
    int numBins  = 1025;
    
    // Reference Standard: 75% Overlap
    const int overlap = 4;
    int hopSize = 512;
    
    int count = 0;
    int pos = 0;

    std::unique_ptr<juce::dsp::FFT> fftEngine;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> windowEngine;

    // Working & FIFO Buffers
    std::vector<float> inputFifo;
    std::vector<float> outputFifo;
    std::vector<float> fftData; // Size: fftSize * 2 for complex interleaving

    double sampleRate = 44100.0;
    float targetCutoffHz = 1000.0f;

    void processFrame();
};