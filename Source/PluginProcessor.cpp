#include "PluginProcessor.h"
#include "CombFilter.h"
#include "PluginEditor.h"

// 1. Implement the parameter layout helper function
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::NormalisableRange<float> feedbackRange (0.0f, 1.0f, 0.01f, 1.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("FEEDBACK", 1), // Parameter ID
        "Feedback",                     // UI Name
        feedbackRange,                         // Range configuration
        0.8f                              // Default value
    ));

    juce::NormalisableRange<float> dampingRange (0.0f, 1.0f, 0.01f, 1.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("DAMPING", 1), // Parameter ID
        "Damping",                     // UI Name
        dampingRange,                         // Range configuration
        0.5f                              // Default value
    ));

    juce::NormalisableRange<float> mixRange (0.0f, 100.0f, 0.1f, 1.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("MIX", 1), // Parameter ID
        "Mix",                     // UI Name
        mixRange,                         // Range configuration
        50.0f                              // Default value
    ));


    // Setup the Chromatic Note Names array
    juce::StringArray noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    
    // Note Selector: 0 to 11, default to 0 (C)
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("NOTE", 1), "Note", noteNames, 0));

    // 2. Setup Octave Selector: 0 to 8 (Octave 4 is Middle C territory), default to 4
    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID ("OCTAVE", 1), "Octave", 0, 8, 4));



    return layout;
}

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
        apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}


void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{


    feedback = apvts.getRawParameterValue("FEEDBACK");
    damping = apvts.getRawParameterValue("DAMPING");
    note = apvts.getRawParameterValue("NOTE");
    octave = apvts.getRawParameterValue("OCTAVE");
    mix = apvts.getRawParameterValue("MIX");

    


    juce::dsp::ProcessSpec filterSpec;
    filterSpec.sampleRate = sampleRate;
    filterSpec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    filterSpec.numChannels = 1; 

    for (int channel = 0; channel < numChannels; ++channel)
    {
        for (int voice = 0; voice < numVoices; ++voice) {
            CombFilter& filter = filterBank[channel][voice];
            filter.prepare (filterSpec); 
            filter.reset(); 
        }   
    }

    midiNotes.fill(-1);
}

void AudioPluginAudioProcessor::releaseResources() {}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Clear unused output channels
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear (i, 0, buffer.getNumSamples());
    }     
    
    // Read your non-pitch parameters
    float currentFeedback = feedback->load();
    float currentDamping = damping->load();
    float currentMix = mix->load() / 100.0f;

    // read live midi inputs
    for (const auto metadata: midiMessages) {
        auto message = metadata.getMessage();
        if (message.isNoteOn()) {
            int note = message.getNoteNumber();

            // find the first free voice
            for (int voice = 0; voice < numVoices; ++voice) {
                if (midiNotes[voice] == -1) {
                    midiNotes[voice] = note;
                    break;
                }
            }
        } else if (message.isNoteOff()) {
            int note = message.getNoteNumber();

            // find voice playing note and mark as unused
            for (int voice = 0; voice < numVoices; ++voice) {
                if (midiNotes[voice] == note) {
                    midiNotes[voice] = -1;
                    break;
                }
            }
        }
        
    }

    std::array<float, numVoices> voiceFrequencies;
    for (int voice = 0; voice < numVoices; ++voice) {
        float nextFreq = -1;
        if (midiNotes[voice] != -1) {
            nextFreq = midiToHz(midiNotes[voice]);
        }
        voiceFrequencies[voice] = nextFreq;
    }

    // Push the updated frequencies directly into the filter configurations
    for (int channel = 0; channel < numChannels; ++channel)
    {
        for (int voice = 0; voice < numVoices; ++voice) {
            CombFilter& filter = filterBank[channel][voice];

            float nextFreq = voiceFrequencies[voice];
            if (nextFreq == -1) {
                filter.setFeedback (0);
            } else {
                filter.setFeedback (currentFeedback);
            }
            filter.setDamping (currentDamping);
            filter.setTargetFrequency(nextFreq);
            
        }   
    }

    // =================================================================
    // MAIN AUDIO PROCESSING LOOP
    // =================================================================
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float inputSample = channelData[sample];

            float summedWetOutput = 0.0f;

            // Only run the processing if the filter has a valid target frequency
            for (int voice = 0; voice < numVoices; ++voice) {
                CombFilter& filter = filterBank[channel][voice];
                summedWetOutput += filter.processSample(inputSample);    
            }

            float scaleFactor = 1.0f / static_cast<float>(numVoices);
            float wetSample = summedWetOutput * scaleFactor;

            float blendedOutput = (inputSample * (1.0f - currentMix)) + (wetSample * currentMix);
            channelData[sample] = blendedOutput;
        }
    }
    
    // Clear the MIDI buffer at the end of the block so messages don't pile up or repeat
    midiMessages.clear();
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}


inline float AudioPluginAudioProcessor::midiToHz (int midiNote) {
    return 440.0f * std::pow (2.0f, (static_cast<float> (midiNote) - 69.0f) / 12.0f);
}

std::array<int, 5> AudioPluginAudioProcessor::getActiveMidiNotes() 
{
    // If you are using std::atomic for midiNotes, load them here.
    // If it's a plain array, we can return a direct copy for local diagnostic view:
    std::array<int, 5> snapshot;
    for (int i = 0; i < 5; ++i) {
        snapshot[i] = midiNotes[i];
    }
    return snapshot;
}