// Source/DSP/SamplerEngine.h
#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <memory>

// ============================================================
// SamplerEngine
//
// A thread-safe audio sample playback engine designed to slot
// into the same excitation path as live audio input.
//
// Audio thread calls: getNextSampleL/R(), triggerPlayback(),
//                     stopPlayback(), advance()
// Message thread calls: loadFile(), all parameter setters
// ============================================================
class SamplerEngine
{
public:
    // --------------------------------------------------------
    // Playback modes
    // --------------------------------------------------------
    enum class PlaybackMode
    {
        ContinuousLoop,       // Loops regardless of MIDI
        KeyTriggerFromStart,  // Triggers from startPoint on note-on
        KeyTriggerFromRandom  // Triggers from random position on note-on
    };

    SamplerEngine();
    ~SamplerEngine();

    // --------------------------------------------------------
    // Message thread — call these from the UI / processor setup
    // --------------------------------------------------------

    // Load a file and decode it to the internal buffer.
    // Returns true on success. Safe to call while audio is running.
    bool loadFile (const juce::File& file, double targetSampleRate);

    // Clear the loaded sample
    void clearSample();

    bool hasSample() const { return activeBuffer.load() != nullptr; }

    // Returns a read-only pointer to the active buffer for UI use.
    // Only call from the message thread.
    const juce::AudioBuffer<float>* getBufferForDisplay() const { return activeBuffer.load(); }
    juce::String getLoadedFileName() const { return loadedFileName; }

    // Encode the current sample buffer to a MemoryBlock for state persistence
    void saveToMemoryBlock (juce::MemoryBlock& destData) const;

    // Decode and load a sample buffer from state data
    bool loadFromMemoryBlock (const void* data, int sizeInBytes, double targetSampleRate);

    // --------------------------------------------------------
    // Parameter setters — safe to call from any thread
    // --------------------------------------------------------
    void setPlaybackMode (PlaybackMode mode)
    {
        playbackMode.store (mode);

        if (mode == PlaybackMode::ContinuousLoop && hasSample())
        {
            readPosition = static_cast<double> (
                static_cast<int> (startPoint.load() * (activeBuffer.load()->getNumSamples() - 1)));
            isPlaying.store (true);
        }
    }
    void setGain           (float gainLinear)   { gain.store (gainLinear);                          }
    void setReverse        (bool shouldReverse) { reverse.store (shouldReverse);                    }
    void setStartPoint     (float normalised)   { startPoint.store (juce::jlimit (0.0f, 1.0f, normalised)); }
    void setEndPoint       (float normalised)   { endPoint.store   (juce::jlimit (0.0f, 1.0f, normalised)); }
    void setPitchSemitones (float semitones)    { pitchSemitones.store (semitones);                 }
    void setTriggerSource  (bool useRawMidi)    { triggerFromRawMidi.store (useRawMidi);            }

    PlaybackMode getPlaybackMode()   const { return playbackMode.load();      }
    float        getGain()           const { return gain.load();              }
    bool         getReverse()        const { return reverse.load();           }
    float        getStartPoint()     const { return startPoint.load();        }
    float        getEndPoint()       const { return endPoint.load();          }
    float        getPitchSemitones() const { return pitchSemitones.load();    }
    bool         getTriggerFromRawMidi() const { return triggerFromRawMidi.load(); }

    // --------------------------------------------------------
    // Audio thread — call these from processBlock
    // --------------------------------------------------------

    // Call on each note-on that should trigger the sampler.
    // Does nothing in ContinuousLoop mode.
    void triggerPlayback();

    // Call on note-off / all-notes-off if you want to stop
    // non-looping playback early
    void stopPlayback() { isPlaying.store (false); }

    // Read the current sample value for L and R channels.
    // Returns 0.0f if no sample is loaded or playback is inactive.
    float getNextSampleL();
    float getNextSampleR();

    // Advance the read position by one sample (with pitch shift).
    // Call this once per sample loop iteration, after getNextSampleL/R.
    void advance (double sampleRate);

    // Normalised read position in [0, 1] — read by UI for playhead
    std::atomic<float> normalisedReadPosition { 0.0f };

    // True if currently playing (always true in ContinuousLoop mode
    // when a sample is loaded)
    std::atomic<bool> isPlaying { false };

private:
    // --------------------------------------------------------
    // Buffer management — double-buffered for thread safety
    // --------------------------------------------------------

    // The active buffer pointer is swapped atomically after loading
    std::atomic<juce::AudioBuffer<float>*> activeBuffer { nullptr };

    // Staging buffer loaded on the message thread, then swapped in
    std::unique_ptr<juce::AudioBuffer<float>> stagingBuffer;
    std::unique_ptr<juce::AudioBuffer<float>> previousBuffer; // kept alive until next swap

    // --------------------------------------------------------
    // Playback state — audio thread only
    // --------------------------------------------------------
    double readPosition    = 0.0; // fractional sample index
    double pitchRatio      = 1.0; // derived from pitchSemitones each advance()

    // --------------------------------------------------------
    // Parameters — atomic for cross-thread safety
    // --------------------------------------------------------
    std::atomic<PlaybackMode> playbackMode { PlaybackMode::ContinuousLoop };
    std::atomic<float>        gain         { 1.0f };
    std::atomic<bool>         reverse      { false };
    std::atomic<float>        startPoint   { 0.0f };
    std::atomic<float>        endPoint     { 1.0f };
    std::atomic<float>        pitchSemitones { 0.0f };
    std::atomic<bool>         triggerFromRawMidi { true };

    juce::String loadedFileName;

    // Linear interpolation between two samples
    float interpolate (const float* data, int numSamples, double position) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};
