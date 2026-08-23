// Source/DSP/SamplerEngine.h
#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <memory>

// ============================================================
// SamplerEngine
//
// A thread-safe audio sample playback engine designed to slot
// into the same excitation path as live audio input.
//
// Playback behaviour is controlled by three independent boolean
// axes rather than a single mode enum, since each is orthogonal:
//
//   loopEnabled  — Loop        (true) vs. One-shot        (false)
//   freeRun      — Free Run    (true) vs. Key Trigger      (false)
//   startRandom  — Start Random(true) vs. Start at Start   (false)
//
// "Free Run" plays continuously from load, ignoring MIDI, and is
// what "Continuous Loop" used to mean on its own. "Key Trigger"
// means only triggerPlayback() (note-on) starts/restarts playback.
// startRandom only matters at the moment playback (re)starts, so
// it only affects triggerPlayback() and the initial free-run start.
//
// Audio thread calls: getNextSampleL/R(), triggerPlayback(),
//                     stopPlayback(), advance()
// Message thread calls: loadFile(), all parameter setters
// ============================================================
class SamplerEngine : public juce::ChangeBroadcaster
{
public:
    SamplerEngine();
    ~SamplerEngine();

    // --------------------------------------------------------
    // Message thread — call these from the UI / processor setup
    // --------------------------------------------------------

    // Load a file and decode it to the internal buffer.
    // Returns true on success. Safe to call while audio is running.
    bool loadFile (const juce::File& file, double targetSampleRate);

    bool loadFromMemoryBlock (const void* data, int sizeInBytes,
                            double targetSampleRate, const juce::String& fileName = {});

    // Clear the loaded sample
    void clearSample();

    bool hasSample() const { return activeBuffer.load() != nullptr; }

    // Bumped every time the active sample buffer is replaced (load/clear).
    // Lets callers (e.g. the processor's getStateInformation) cheaply detect
    // whether the sample actually changed since last time, instead of
    // unconditionally re-encoding the whole buffer on every call.
    std::uint32_t getSampleVersion() const { return sampleVersion.load(); }

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

    // Loop vs. one-shot: whether reaching the trim end wraps back to the
    // trim start (loop) or stops playback (one-shot).
    void setLoopEnabled (bool shouldLoop) { loopEnabled.store (shouldLoop); }

    // Free-run vs. key-triggered. Switching into free-run immediately starts
    // playback from the trim start (mirroring what the old ContinuousLoop
    // mode did on load); switching into key-trigger stops playback until the
    // next note-on, since triggerPlayback() is now the only thing that can
    // start it.
    void setFreeRun (bool shouldFreeRun)
    {
        freeRun.store (shouldFreeRun);

        if (shouldFreeRun)
        {
            if (hasSample())
                readPosition = static_cast<double> (
                    static_cast<int> (startPoint.load() * (activeBuffer.load()->getNumSamples() - 1)));

            isPlaying.store (hasSample());
        }
        else
        {
            isPlaying.store (false);
        }
    }

    // Start-at-start vs. start-random. Only affects where playback begins
    // the next time it (re)starts — triggerPlayback() in key-trigger mode,
    // or the immediate start applied by setFreeRun(true) above.
    void setStartRandom (bool shouldStartRandom) { startRandom.store (shouldStartRandom); }

    void setGain           (float gainLinear)   { gain.store (gainLinear);                          }
    void setReverse        (bool shouldReverse) { reverse.store (shouldReverse);                    }
    void setStartPoint     (float normalised)   { startPoint.store (juce::jlimit (0.0f, 1.0f, normalised)); }
    void setEndPoint       (float normalised)   { endPoint.store   (juce::jlimit (0.0f, 1.0f, normalised)); }
    void setReadPos        (float normalised)   {readPosition = juce::jlimit(0.0f, 1.0f, normalised); }
    void setPitchSemitones (float semitones)    { pitchSemitones.store (semitones);                 }

    bool         getLoopEnabled()    const { return loopEnabled.load();      }
    bool         getFreeRun()        const { return freeRun.load();          }
    bool         getStartRandom()    const { return startRandom.load();      }
    float        getGain()           const { return gain.load();              }
    bool         getReverse()        const { return reverse.load();           }
    float        getStartPoint()     const { return startPoint.load();        }
    float        getEndPoint()       const { return endPoint.load();          }
    float        getPitchSemitones() const { return pitchSemitones.load();    }


    // --------------------------------------------------------
    // Audio thread — call these from processBlock
    // --------------------------------------------------------

    // Call on each note-on that should trigger the sampler.
    // Does nothing in free-run mode, since free-run ignores MIDI entirely.
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

    // True if currently playing
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
    std::atomic<bool>         loopEnabled  { true };
    std::atomic<bool>         freeRun      { true };
    std::atomic<bool>         startRandom  { false };
    std::atomic<float>        gain         { 1.0f };
    std::atomic<bool>         reverse      { false };
    std::atomic<float>        startPoint   { 0.0f };
    std::atomic<float>        endPoint     { 1.0f };
    std::atomic<float>        pitchSemitones { 0.0f };
    std::atomic<bool>         triggerFromRawMidi { true };

    juce::String loadedFileName;
    std::atomic<std::uint32_t> sampleVersion { 0 };

    // Linear interpolation between two samples
    float interpolate (const float* data, int numSamples, double position) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEngine)
};