#pragma once
#include "FFTProcessor.h"
#include "CombFilter.h"
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

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}

    void getStateInformation (juce::MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

    std::array<int, 5> getActiveMidiNotes() ;


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();


    inline float midiToHz (int midiNote);

    static constexpr int numVoices = 5;
    static constexpr int numChannels = 2;

    std::array<std::array<CombFilter, numVoices>, numChannels> filterBank;
    
    std::array<int, numVoices> midiNotes;

    std::atomic<float>* feedback;
    std::atomic<float>* mix;
    std::atomic<float>* damping;
    std::atomic<float>* note;
    std::atomic<float>* octave;
};