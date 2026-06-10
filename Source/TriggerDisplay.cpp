// Source/TriggerDisplay.cpp
#include "TriggerDisplay.h"
#include "ResonatorPalette.h"

TriggerDisplay::TriggerDisplay (AudioPluginAudioProcessor& processor)
    : proc (processor)
{
    // Compute how many decimated ring buffer samples span displaySeconds.
    // The processor writes one sample per noiseDecimationFactor audio samples,
    // so the effective sample rate of the ring buffer is:
    //   sampleRate / noiseDecimationFactor
    // We don't have the sample rate here at construction time, so we use a
    // sensible default (44100) and recompute in timerCallback if the buffer
    // size looks wrong.  In practice the display looks fine either way since
    // it's purely cosmetic.
    constexpr float assumedSampleRate = 44100.0f;
    const float ringRate = assumedSampleRate
                           / static_cast<float> (AudioPluginAudioProcessor::noiseDecimationFactor
                                                 * displayDecimation);

    numDisplaySamples = static_cast<int> (ringRate * displaySeconds);
    numDisplaySamples = juce::jmin (numDisplaySamples,
                                    AudioPluginAudioProcessor::noiseRingSize);

    displayBuffer.resize (numDisplaySamples, 0.0f);

    startTimerHz (repaintRateHz);
}

TriggerDisplay::~TriggerDisplay()
{
    stopTimer();
}

// ============================================================
// Timer — copy ring buffer snapshot, trigger repaint
// ============================================================

void TriggerDisplay::timerCallback()
{
    const int ringSize = AudioPluginAudioProcessor::noiseRingSize;
    const int writePos = proc.noiseWritePos.load (std::memory_order_acquire);

    // Initialise lastReadPos on first tick
    if (lastReadPos < 0)
    {
        lastReadPos = writePos;
        repaint();
        return;
    }

    // Drain all new ring buffer samples since our last read
    while (lastReadPos != writePos)
    {
        const float sample = proc.noiseRingBuffer[lastReadPos].load (std::memory_order_relaxed);
        lastReadPos = (lastReadPos + 1) % ringSize;

        // Accumulate peak within the decimation window
        pendingPeak = juce::jmax (pendingPeak, sample);
        ++decimationCounter;

        if (decimationCounter >= displayDecimation)
        {
            // Shift display buffer left by one and append the new peak
            std::rotate (displayBuffer.begin(), displayBuffer.begin() + 1, displayBuffer.end());
            displayBuffer.back() = pendingPeak;

            pendingPeak     = 0.0f;
            decimationCounter = 0;
        }
    }

    repaint();
}

// ============================================================
// Paint
// ============================================================

void TriggerDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (6.0f);

    // Background
    g.setColour (ResonatorPalette::backgroundWidget());
    g.fillRoundedRectangle (bounds, 4.0f);

    if (displayBuffer.empty())
        return;

    const float innerX = bounds.getX() + 4.0f;
    const float innerY = bounds.getY() + 4.0f;
    const float innerW = bounds.getWidth()  - 8.0f;
    const float innerH = bounds.getHeight() - 8.0f;
    const float midY   = innerY + innerH * 0.5f;

    const int   n     = static_cast<int> (displayBuffer.size());
    const float xStep = innerW / static_cast<float> (n - 1);

    // Build the filled waveform path (top half mirrored to bottom)
    juce::Path filledPath;
    filledPath.startNewSubPath (innerX, midY);

    for (int i = 0; i < n; ++i)
    {
        const float x       = innerX + static_cast<float> (i) * xStep;
        const float amp     = juce::jlimit (0.01f, 1.0f, displayBuffer[i]);
        const float yOffset = amp * (innerH * 0.5f);
        filledPath.lineTo (x, midY - yOffset);
    }

    for (int i = n - 1; i >= 0; --i)
    {
        const float x       = innerX + static_cast<float> (i) * xStep;
        const float amp     = juce::jlimit (0.01f, 1.0f, displayBuffer[i]);
        const float yOffset = amp * (innerH * 0.5f);
        filledPath.lineTo (x, midY + yOffset);
    }

    filledPath.closeSubPath();

    // Threshold line
    g.setColour (ResonatorPalette::accentSecondary());
    const float threshDB     = proc.apvts.getRawParameterValue ("TRIG_THRESHOLD")->load();
    const float threshLinear = juce::Decibels::decibelsToGain (threshDB);
    const float threshY      = midY - juce::jlimit (0.0f, 1.0f, threshLinear) * (innerH * 0.5f);

    g.setColour (ResonatorPalette::accentSecondary());
    g.drawHorizontalLine (static_cast<int> (threshY), innerX, innerX + innerW);
    float gradientAlpha = 0.8f;
    juce::ColourGradient waveGradient (
        ResonatorPalette::accentSecondary().withAlpha(gradientAlpha),                          // top peaks
        0.0f, innerY - 20.0f,
        ResonatorPalette::accentPrimary().withAlpha(gradientAlpha),   // centre line
        0.0f, midY,
        false
    );

    g.setGradientFill (waveGradient);
    g.fillPath (filledPath);
    

    
}
