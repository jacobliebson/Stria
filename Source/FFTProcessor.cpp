#include "FFTProcessor.h"
#include <cmath>

void FFTProcessor::orderChanged (int newFFTOrder, double newSampleRate)
{
    sampleRate = newSampleRate;
    fftOrder = newFFTOrder;
    fftSize = 1 << fftOrder;
    numBins = (fftSize / 2) + 1;
    
    // Establish 75% Overlap metrics
    hopSize = fftSize / overlap;
    
    fftEngine = std::make_unique<juce::dsp::FFT> (fftOrder);
    
    // Reference uses size + 1 to handle periodic window conversions cleanly
    windowEngine = std::make_unique<juce::dsp::WindowingFunction<float>> (
        (size_t)(fftSize + 1), 
        juce::dsp::WindowingFunction<float>::hann, 
        false
    );
    
    inputFifo.assign ((size_t)fftSize, 0.0f);
    outputFifo.assign ((size_t)fftSize, 0.0f);
    fftData.assign ((size_t)(fftSize * 2), 0.0f);
    
    count = 0;
    writeIndex = 0;
    readIndex = 0;
}

void FFTProcessor::setSpecThresh(float newSpecThresh) {
    specThresh = newSpecThresh;
}

int FFTProcessor::getFFTSize() {
    return fftSize;
}

void FFTProcessor::pushSample (float sample)
{
    if (inputFifo.size() == 0) return;

    // Write strictly to the write tracker
    inputFifo[(size_t)writeIndex] = sample;
    writeIndex = (writeIndex + 1) % fftSize;
    
    // Count hop blocks independently of the raw pointer address
    count += 1;
    if (count >= hopSize)
    {
        count = 0;
        processFrame();
    }
}

float FFTProcessor::popSample()
{
    if (outputFifo.size() == 0) return 0.0f;

    // Read strictly from the read tracker
    float outputSample = outputFifo[(size_t)readIndex];
    
    // Clear the canvas tail
    outputFifo[(size_t)readIndex] = 0.0f;
    
    readIndex = (readIndex + 1) % fftSize;
    
    return outputSample;
}

void FFTProcessor::processFrame()
{
    std::fill (fftData.begin(), fftData.end(), 0.0f);

    float* fftPtr = fftData.data();
    const float* inputPtr = inputFifo.data();
    
    // Snapshot the current write pointer position for this frame calculation
    int fftStartPos = writeIndex; 

    // Unwrap circular buffer using the frozen snapshot
    std::memcpy (fftPtr, inputPtr + fftStartPos, (size_t)(fftSize - fftStartPos) * sizeof (float));
    if (fftStartPos > 0)
    {
        std::memcpy (fftPtr + (fftSize - fftStartPos), inputPtr, (size_t)fftStartPos * sizeof (float));
    }

    // Window -> FFT -> Gate -> iFFT -> Window
    windowEngine->multiplyWithWindowingTable (fftPtr, (size_t)fftSize);
    fftEngine->performRealOnlyForwardTransform (fftPtr, true);
    
    processFrequencyDomain();
    
    fftEngine->performRealOnlyInverseTransform (fftPtr);
    windowEngine->multiplyWithWindowingTable (fftPtr, (size_t)fftSize);

    // Accumulate back into circular buffer using the isolated snapshot
    const float windowCorrection = 2.0f / 3.0f; 
    
    for (int i = 0; i < fftStartPos; ++i) 
    {
        outputFifo[(size_t)i] += fftPtr[i + fftSize - fftStartPos] * windowCorrection;
    }
    for (int i = 0; i < fftSize - fftStartPos; ++i) 
    {
        outputFifo[(size_t)(i + fftStartPos)] += fftPtr[i] * windowCorrection;
    }
}

void FFTProcessor::processFrequencyDomain()
{
    // Total number of unique frequency bins (N/2 + 1)
    const int numBins = (fftSize / 2) + 1;
    
    // Calculate the width of a single spectral bin (f_s / N)
    const float binWidth = static_cast<float> (sampleRate) / static_cast<float> (fftSize);
    
    // Reinterpret the float array as a collection of complex numbers
    // This maps: [real0, imag0, real1, imag1...] -> [complexBin0, complexBin1...]
    auto* complexBins = reinterpret_cast<std::complex<float>*> (fftData.data());

    // Loop through every bin from 0 Hz up to the Nyquist frequency (sampleRate / 2)
    for (int bin = 0; bin < numBins; ++bin)
    {
        // 1. Calculate the exact center frequency of this specific bin in Hz
        float binFrequencyHz = static_cast<float> (bin) * binWidth;
        
        // Skip DC offset (0 Hz) and Nyquist to avoid altering fundamental symmetry
        if (bin == 0 || bin == numBins - 1)
            continue;

        // 2. Extract the current complex spectral value
        std::complex<float> complexValue = complexBins[bin];
        
        // Extract amplitude/magnitude and phase angle
        float magnitude = std::abs (complexValue);
        float phase     = std::arg (complexValue);

        float specThreshLinear = juce::Decibels::decibelsToGain(specThresh);
        
        if (magnitude < specThreshLinear) {
            magnitude = 0.0f;
        }

       
        // =================================================================
        // 3. Reconstruct and commit the modified complex data back to the array
        // =================================================================
        
        complexBins[bin] = std::polar (magnitude, phase);
        

    }
}