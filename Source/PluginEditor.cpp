#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdlib>
#include <string>

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300); // Sets the window size
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkgrey); // Clean, dark background
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    std::string text =  std::to_string(audioProcessor.getActiveMidiNotes()[0]) + " " + 
                        std::to_string(audioProcessor.getActiveMidiNotes()[1]) + " " + 
                        std::to_string(audioProcessor.getActiveMidiNotes()[2]) + " " +
                        std::to_string(rand());
    g.drawFittedText (text, getLocalBounds(), juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized() {}