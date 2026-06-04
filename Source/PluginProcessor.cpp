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
        0.5f                              // Default value
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
{}

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
}

void AudioPluginAudioProcessor::releaseResources() {}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
   
    // clear unused channels
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear (i, 0, buffer.getNumSamples());
    }     
    
     // Read your parameters here 
    float currentFeedback = feedback->load();
    float currentDamping = damping->load();
    int noteIndex = static_cast<int>(note->load());
    int currentOctave = static_cast<int>(octave->load());
    float currentMix = mix->load() / 100.0f;

    // Calculate the target frequency
    // (octaveValue + 1) aligns Octave 4 to Middle C (MIDI Note 60) when Note is 0 (C)
    int absoluteMidiNote = ((currentOctave + 1) * 12) + noteIndex;

    int root = absoluteMidiNote;
    int third = absoluteMidiNote + 4;
    int fifth = absoluteMidiNote + 7;
    
    // Standard MIDI to Frequency formula: 440.0 * pow(2.0, (midiNote - 69) / 12.0)
    float rootFrequency = midiToHz(root);
    float thirdFrequency = midiToHz(third);
    float fifthFrequency = midiToHz(fifth);

    std::array<float, 3> voiceFrequencies;
    voiceFrequencies[0] = rootFrequency;
    voiceFrequencies[1] = thirdFrequency;
    voiceFrequencies[2] = fifthFrequency;


    for (int channel = 0; channel < numChannels; ++channel)
    {
        for (int voice = 0; voice < numVoices; ++voice) {
            CombFilter& filter = filterBank[channel][voice];
            filter.setFeedback (currentFeedback);
            filter.setDamping (currentDamping);
            filter.setTargetFrequency(voiceFrequencies[voice]);
        }   
    }


    // 4. Process the audio data sample-by-sample
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // Get a direct pointer to the audio data for this specific channel
        auto* channelData = buffer.getWritePointer (channel);


        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            
            float inputSample = channelData[sample];




            float summedWetOutput = 0.0f;

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