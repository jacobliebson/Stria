#include "PluginProcessor.h"
#include "PluginEditor.h"

// 1. Implement the parameter layout helper function
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::NormalisableRange<float> outputGainRange (-12.0f, 12.0f, 0.1f, 1.0f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("output_gain", 1), // Parameter ID
        "Output gain",                     // UI Name
        outputGainRange,                         // Range configuration
        0.0f                              // Default value
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

void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    leftFFTProcessor.orderChanged (11, sampleRate); // 2048 block frame sizing
    rightFFTProcessor.orderChanged (11, sampleRate);
}

void AudioPluginAudioProcessor::releaseResources() {}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Read your parameters here 

    
    // Update both channel processors here


    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* channelDataLeft  = buffer.getWritePointer(0);
    auto* channelDataRight = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // 1. Push current time-domain audio samples into the processors
        leftFFTProcessor.pushSample (channelDataLeft[sample]);
        rightFFTProcessor.pushSample (channelDataRight[sample]);
        
        // 2. Instantly pop out reconstructed frequency-domain treated samples
        channelDataLeft[sample]  = leftFFTProcessor.popSample();
        channelDataRight[sample] = rightFFTProcessor.popSample();
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