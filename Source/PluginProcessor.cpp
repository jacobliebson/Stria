#include "PluginProcessor.h"
#include "PluginEditor.h"

// 1. Implement the parameter layout helper function
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Add a default placeholder parameter so the layout isn't completely empty
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("master_gain", 1), // Parameter ID and version
        "Master Gain",                        // Parameter name shown in DAW
        0.0f,                                 // Minimum value
        1.0f,                                 // Maximum value
        0.5f                                  // Default value
    ));

    return layout;
}

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
        apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void AudioPluginAudioProcessor::releaseResources() {}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clean clear leftover output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Simple pass-through: Copy input to output
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        // Do DSP work here
    }
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}