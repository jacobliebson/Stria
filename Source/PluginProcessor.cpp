#include "PluginProcessor.h"
#include "PluginEditor.h"

// 1. Implement the parameter layout helper function
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::NormalisableRange<float> cutoffRange (20.0f, 20000.0f, 1.0f, 0.3f);

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("fft_cutoff", 1), // Parameter ID
        "Filter Cutoff",                     // UI Name
        cutoffRange,                         // Range configuration
        1000.0f                              // Default value (1 kHz)
    ));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("fft_delta", 1), // Parameter ID
        "Delta Mode",                       // UI Name
        false                               // Default state (Off)
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
    
    // Read your parameters atomically
    float currentCutoff = *apvts.getRawParameterValue ("fft_cutoff");
    bool currentDelta   = *apvts.getRawParameterValue ("fft_delta") > 0.5f; // Cast float to bool
    
    // Update both channel processors
    leftFFTProcessor.setCutoffFrequency (currentCutoff);
    leftFFTProcessor.setDeltaMode (currentDelta);
    
    rightFFTProcessor.setCutoffFrequency (currentCutoff);
    rightFFTProcessor.setDeltaMode (currentDelta);

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