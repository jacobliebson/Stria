#include "PluginProcessor.h"
#include "CombFilter.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    juce::NormalisableRange<float> feedbackRange (-1.0f, 1.0f, 0.01f, 1.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("FEEDBACK", 1), 
        "Feedback",                     
        feedbackRange,                  
        0.8f                            
    ));

    juce::NormalisableRange<float> dampingRange (0.0f, 1.0f, 0.01f, 1.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("DAMPING", 1), 
        "Damping",                     
        dampingRange,                  
        0.5f                            
    ));

    juce::NormalisableRange<float> mixRange (0.0f, 100.0f, 0.1f, 1.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("MIX", 1), 
        "Mix",                     
        mixRange,                  
        80.0f                      
    ));

    // 1. Attack Range: 0.1ms to 100ms, skewed heavily toward the low end (0.35)
    juce::NormalisableRange<float> attackRange (0.1f, 100.0f, 0.1f);
    attackRange.setSkewForCentre(5.0f); // Centers the slider physically around 5.0ms

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("ATTACK", 1),
        "Transient Attack",
        attackRange,
        5.0f // Default 5ms
    ));

    // 2. Release Range: 10ms to 2000ms, skewed toward the low end (0.4)
    juce::NormalisableRange<float> releaseRange (10.0f, 2000.0f, 1.0f);
    releaseRange.setSkewForCentre(250.0f); // Centers the slider physically around 250ms

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("RELEASE", 1),
        "Transient Release",
        releaseRange,
        250.0f // Default 250ms
    ));

    juce::NormalisableRange<float> thresholdRange (0.0f, 24.0f, 0.01f, 1.0f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("THRESHOLD", 1), 
        "Threshold",                     
        thresholdRange,                  
        0.0f                            
    ));

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

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    feedback = apvts.getRawParameterValue("FEEDBACK");
    damping = apvts.getRawParameterValue("DAMPING");
    mix = apvts.getRawParameterValue("MIX");
    attack = apvts.getRawParameterValue("ATTACK");
    release = apvts.getRawParameterValue("RELEASE");
    thresh = apvts.getRawParameterValue("THRESHOLD");

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
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear (i, 0, buffer.getNumSamples());
    }     
    
    float currentFeedback = feedback->load();
    float currentDamping = damping->load();
    float currentMix = mix->load() / 100.0f;
    float attackMs = attack->load();
    float releaseMs = release->load();
    float currentThresh = thresh->load();

    float attackCoef = calculateCoef(attackMs, getSampleRate());
    float releaseCoef = calculateCoef(releaseMs, getSampleRate());

    // Read live MIDI inputs
    for (const auto metadata: midiMessages) {
        auto message = metadata.getMessage();
        if (message.isNoteOn()) {
            int note = message.getNoteNumber();

            // Find the first free voice
            for (int voice = 0; voice < numVoices; ++voice) {
                if (midiNotes[voice] == -1) {
                    midiNotes[voice] = note;
                    break;
                }
            }
        } else if (message.isNoteOff()) {
            int note = message.getNoteNumber();

            // Find voice playing note and mark as unused
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
                filter.setTargetFrequency(440.0f);
            } else {
                filter.setFeedback (currentFeedback);
                filter.setTargetFrequency(nextFreq);
            }
            filter.setDamping (currentDamping);
        }   
    }

    // Transient configuration variables
    
    float thresholdLinear = juce::Decibels::decibelsToGain(currentThresh);

    // Create a safe snapshot of the incoming buffer for envelope tracking and clean dry signals
    juce::AudioBuffer<float> dryCopy;
    dryCopy.makeCopyOf(buffer);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float inputL = dryCopy.getSample(0, sample);
        float absInput = std::abs(inputL);

        envFast = (absInput > envFast) ? (attackCoef * envFast) + ((1.0f - attackCoef) * absInput) 
                                       : (releaseCoef * envFast) + ((1.0f - releaseCoef) * absInput);
                                       
        envSlow = (releaseCoef * envSlow) + ((1.0f - releaseCoef) * absInput);

        bool isTransientActive = (envFast > (envSlow * thresholdLinear));

        float envelopeRatio = (envSlow > 0.0001f) ? (envFast / envSlow) : 0.0f;
        float envelopeGain = std::fmax(0.0f, std::fmin((envelopeRatio - thresholdLinear), 1.0f));

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            float rawDrySample = dryCopy.getSample(channel % numChannels, sample);
            float filterExcitation = rawDrySample * envelopeGain;
            float summedWetOutput = 0.0f;

            if (channel < numChannels)
            {
                for (int voice = 0; voice < numVoices; ++voice) 
                {
                    summedWetOutput += filterBank[channel][voice].processSample(filterExcitation);
                }
            }

            float scaleFactor = 1.0f / static_cast<float>(numVoices);
            float wetSample = summedWetOutput * scaleFactor;

            float blendedOutput = (rawDrySample * (1.0f - currentMix)) + (wetSample * currentMix);
            
            buffer.setSample(channel, sample, blendedOutput);
        }
    }

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

float AudioPluginAudioProcessor::calculateCoef(float timeMs, double sampleRate)
{
    if (timeMs <= 0.0f) return 0.0f;
    return std::exp(-1.0f / (static_cast<float>(sampleRate) * (timeMs / 1000.0f)));
}

std::array<int, 8> AudioPluginAudioProcessor::getActiveMidiNotes() 
{
    std::array<int, 8> snapshot;
    for (int i = 0; i < 8; ++i) {
        snapshot[i] = midiNotes[i];
    }
    return snapshot;
}