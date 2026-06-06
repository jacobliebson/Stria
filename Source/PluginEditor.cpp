#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdlib>
#include <string>

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
    
    // Start the timer to poll for note updates every 33 milliseconds (~30 Hz)
    startTimerHz (30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer(); // Always clean up your timers on destruction!
}

void AudioPluginAudioProcessorEditor::timerCallback()
{

}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void AudioPluginAudioProcessorEditor::resized() {}

