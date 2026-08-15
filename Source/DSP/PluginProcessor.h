#pragma once

// DSP Libraries
#include "Arpeggiator.h"
#include "CombFilter.h"
#include "CustomADSR.h"
#include "ResonatorVoice.h"
#include "SamplerEngine.h"

// Utilities
#include "../Utils/HaltonGenerator.h"
#include "../Utils/Parameters.h"

// JUCE
#include <juce_audio_processors/juce_audio_processors.h>
#include <fstream>
#include <juce_core/juce_core.h>

class AudioPluginAudioProcessor  : public juce::AudioProcessor
{
public:
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override { juce::ignoreUnused (index); }
    const juce::String getProgramName (int index) override { juce::ignoreUnused (index); return {}; }
    void changeProgramName (int index, const juce::String& newName) override { juce::ignoreUnused (index); juce::ignoreUnused (newName); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static constexpr int numArpVoices   = 32;
    static constexpr int numChordVoices = 16;

    std::array<int, numArpVoices + numChordVoices> getActiveMidiNotes();

    juce::AudioProcessorValueTreeState apvts;

    struct ActiveNoteInfo {
        float frequency;
        float amplitude;   // 0–1, from voice envelope
        bool  isArp;
    };

    // Ring buffer — written audio thread, read GUI thread
    static constexpr int noiseRingSize = 4096;
    std::array<std::atomic<float>, noiseRingSize> noiseRingBuffer;
    std::atomic<int> noiseWritePos { 0 };
    static constexpr int noiseDecimationFactor = 32;

    // Active notes snapshot — written audio thread, read GUI thread
    static constexpr int maxActiveNotes = 48; // numArpVoices + numChordVoices
    std::array<ActiveNoteInfo, maxActiveNotes> activeNoteSnapshot;
    std::atomic<int> activeNoteCount { 0 };

    std::atomic<float> gateValue { 0.0f };

    SamplerEngine sampler;
    double sampleRate;

    // Cache of the last base64-encoded sample block written into state, keyed
    // by SamplerEngine::sampleVersion. getStateInformation() can be called by
    // the host very frequently (e.g. on every parameter change, for undo
    // history) — re-encoding the whole sample buffer every time caused
    // audible lag spikes. We only redo the (expensive) encode when the
    // sample itself has actually changed.
    juce::String cachedSampleStateBase64;
    std::uint32_t cachedSampleVersion = 0xFFFFFFFFu;


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)

    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    inline float midiToHz (int midiNote);

    static constexpr int numChannels = 2;
    

    
    std::atomic<bool> useSampler {true};
    std::atomic<float>* audioSourceParam = nullptr;

    // Separate voice pools for arp and chord streams.
    // NOTE: When independent envelopes are added, pass a VoiceRole to
    // updateParameters so each pool can receive different envParams.
    juce::Synthesiser arpSynth;
    juce::Synthesiser chordSynth;
    HaltonGenerator haltonPanner;

    std::atomic<bool> isSustainPressed {false};
    bool currentSustainState = false;

    Arpeggiator arp;
    std::atomic<float>* arpRateIndex        = nullptr;
    std::atomic<float>* arpGateParam        = nullptr;
    std::atomic<float>* arpModeParam        = nullptr;
    std::atomic<float>* arpScatter          = nullptr;
    std::atomic<float>* arpDeviation        = nullptr;
    std::atomic<float>* octRange            = nullptr;

    bool   wasPlaying       = false;
    double lastProcessorPPQ = -1.0;

    int noiseDecimationCounter = 0;

    std::atomic<float>* feedback            = nullptr;
    std::atomic<float>* damping             = nullptr;
    std::atomic<float>* detune              = nullptr;
    std::atomic<float>* detuneMode          = nullptr;

    std::atomic<float>* arpGainDB           = nullptr;
    std::atomic<float>* chordGainDB         = nullptr;
    std::atomic<float>* mix                 = nullptr;
    std::atomic<float>* spread              = nullptr;
    std::atomic<float>* arpPan              = nullptr;
    std::atomic<float>* chordPan            = nullptr;

    CustomADSR gateEnvelope;
    std::atomic<float>* trigThreshold = nullptr;
    std::atomic<float>* trigAttack      = nullptr;
    std::atomic<float>* trigHold        = nullptr;
    std::atomic<float>* trigRelease     = nullptr;
    std::atomic<float>* legatoModeParam = nullptr;
    bool wasAboveThreshold = false;

    std::atomic<float>* arpEnvAttack        = nullptr;
    std::atomic<float>* arpEnvDecay         = nullptr;
    std::atomic<float>* arpEnvSustain       = nullptr;
    std::atomic<float>* arpEnvRelease       = nullptr;

    std::atomic<float>* chordEnvAttack      = nullptr;
    std::atomic<float>* chordEnvDecay       = nullptr;
    std::atomic<float>* chordEnvSustain     = nullptr;
    std::atomic<float>* chordEnvRelease     = nullptr;


    

    
    float calculateCoef (float timeMs, double sampleRate);
};