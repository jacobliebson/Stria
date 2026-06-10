#pragma once
#include "Arpeggiator.h"
#include "CombFilter.h"
#include "Parameters.h"
#include "CustomADSR.h"
#include <juce_audio_processors/juce_audio_processors.h>

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

    void getStateInformation (juce::MemoryBlock& destData) override { juce::ignoreUnused (destData); }
    void setStateInformation (const void* data, int sizeInBytes) override { juce::ignoreUnused (data); juce::ignoreUnused (sizeInBytes); }

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


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)

    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    inline float midiToHz (int midiNote);

    static constexpr int numChannels = 2;

    // Separate voice pools for arp and chord streams.
    // NOTE: When independent envelopes are added, pass a VoiceRole to
    // updateParameters so each pool can receive different envParams.
    juce::Synthesiser arpSynth;
    juce::Synthesiser chordSynth;

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

    std::atomic<float>* mix                 = nullptr;
    std::atomic<float>* arpGainDB           = nullptr;
    std::atomic<float>* chordGainDB         = nullptr;

    CustomADSR gateEnvelope;
    std::atomic<float>* trigThreshold = nullptr;
    std::atomic<float>* trigAttack      = nullptr;
    std::atomic<float>* trigHold        = nullptr;
    std::atomic<float>* trigRelease     = nullptr;
    bool wasAboveThreshold;

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