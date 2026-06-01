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
    pos = 0;
}

void FFTProcessor::setCutoffFrequency (float frequencyInHz)
{
    targetCutoffHz = juce::jlimit (20.0f, static_cast<float> (sampleRate / 2.0), frequencyInHz);
}

void FFTProcessor::setDeltaMode (bool shouldBeDelta)
{
    isDeltaModeActive = shouldBeDelta;
}

void FFTProcessor::pushSample (float sample)
{
    if (inputFifo.size() == 0) return;

    // 1. Push sample into the circular input buffer
    inputFifo[(size_t)pos] = sample;
    
    // 2. Count ticks up toward our Hop Size boundary (75% overlap threshold)
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

    // 3. Extract sample from the output circular canvas
    float outputSample = outputFifo[(size_t)pos];
    
    // Clear out stale values so they don't corrupt subsequent accumulations
    outputFifo[(size_t)pos] = 0.0f;
    
    // 4. Advance both FIFOs simultaneously using the unified circular tracker
    pos = (pos + 1) % fftSize;
    
    return outputSample;
}

void FFTProcessor::processFrame()
{
    // Clear the complex working workspace entirely
    std::fill (fftData.begin(), fftData.end(), 0.0f);

    // 5. Unwrap circular buffer chronologically into the front half of the FFT array
    float* fftPtr = fftData.data();
    const float* inputPtr = inputFifo.data();
    
    std::memcpy (fftPtr, inputPtr + pos, (size_t)(fftSize - pos) * sizeof (float));
    if (pos > 0)
    {
        std::memcpy (fftPtr + (fftSize - pos), inputPtr, (size_t)pos * sizeof (float));
    }

    // 6. Apply Analysis Windowing Function
    windowEngine->multiplyWithWindowingTable (fftPtr, (size_t)fftSize);

    // 7. Execute Real Forward FFT
    fftEngine->performRealOnlyForwardTransform (fftPtr, true);

    // 8. Process Spectral Filtering Block
    processFrequencyDomain();

    // 9. Execute Real Inverse Transform (JUCE scales this natively by 1/N)
    fftEngine->performRealOnlyInverseTransform (fftPtr);

    // 10. Apply Synthesis Windowing Function
    windowEngine->multiplyWithWindowingTable (fftPtr, (size_t)fftSize);

    // 11. Accumulate back into Circular Output Buffer with COLA Amplitude Correction
    const float windowCorrection = 2.0f / 3.0f; // Eliminates 1.5x gain of 75% Hann Overlap
    
    for (int i = 0; i < pos; ++i) 
    {
        outputFifo[(size_t)i] += fftPtr[i + fftSize - pos] * windowCorrection;
    }
    for (int i = 0; i < fftSize - pos; ++i) 
    {
        outputFifo[(size_t)(i + pos)] += fftPtr[i] * windowCorrection;
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
        
        // Normalize the relationship: 0.0 at 0Hz, 1.0 at the cutoff frequency
        float normalizedDistance = binFrequencyHz / targetCutoffHz;

        // Ensure we clamp it between 0.0 and 1.0 so we don't take sqrt of a negative,
        // and frequencies ABOVE the cutoff remain completely untouched (multiplied by 1.0)
        normalizedDistance = juce::jlimit (0.0f, 1.0f, normalizedDistance);

        // Apply the square-root scaling curve
        float mag1 = magnitude * std::pow (normalizedDistance, 0.5f);

        if (isDeltaModeActive) {
            magnitude -= mag1;
        } else {
            magnitude = mag1;
        }

       
        // =================================================================
        // 3. Reconstruct and commit the modified complex data back to the array
        // =================================================================
        
        complexBins[bin] = std::polar (magnitude, phase);
        

    }
}