// Source/TriggerDisplay.h
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ResonatorPalette.h"

// ============================================================
// TriggerDisplay
//
// A scrolling waveform display fed by a lock-free ring buffer
// written on the audio thread in PluginProcessor.
//
// Tuning constants (adjust to taste):
//   displaySeconds   — how much time the full width represents
//   decimationFactor — how many audio samples are averaged into
//                      one display sample (controls resolution
//                      vs. CPU cost; lower = higher resolution)
// ============================================================
class TriggerDisplay : public juce::Component,
                       private juce::Timer
{
public:
    explicit TriggerDisplay (AudioPluginAudioProcessor& processor);
    ~TriggerDisplay() override;

    void paint  (juce::Graphics&) override;
    void resized() override {}

private:
    void timerCallback() override;

    // ------------------------------------------------------------------
    // Display parameters — edit these to tune the visualizer
    // ------------------------------------------------------------------

    // Width of the time window shown, in seconds
    static constexpr float displaySeconds = 2.0f;

    // One display sample = this many audio samples written to the ring buffer.
    // The ring buffer is already decimated by noiseDecimationFactor (32) in
    // the processor, so the effective audio decimation is:
    //   noiseDecimationFactor * displayDecimation
    // At 44.1 kHz with noiseDecimationFactor=32 and displayDecimation=1,
    // the ring buffer fills at ~1378 samples/sec, giving ~2756 samples for
    // a 2-second window — comfortably more than any display width.
    // Increase displayDecimation if you want a coarser/cheaper display.
    static constexpr int displayDecimation = 10;

    // Repaint rate in Hz — 30 is smooth enough for a waveform scroller
    static constexpr int repaintRateHz = 30;

    // ------------------------------------------------------------------
    // Internal state
    // ------------------------------------------------------------------
    // Replace the existing private section members with:
    AudioPluginAudioProcessor& proc;

    std::vector<float> displayBuffer;
    int numDisplaySamples = 0;
    int lastReadPos       = -1;  // last ring buffer index we consumed
    int decimationCounter = 0;   // counts ring buffer samples since last display sample
    float pendingPeak     = 0.0f; // peak accumulator between display samples
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerDisplay)
};
