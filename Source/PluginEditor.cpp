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
    // Grab the live snapshot from the processor
    auto currentNotes = audioProcessor.getActiveMidiNotes();
    
    // If the data has changed since the last frame, force a visual repaint
    if (currentNotes != localMidiNotes)
    {
        localMidiNotes = currentNotes;
        repaint(); 
    }
}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Clear background to a dark grey
    g.fillAll (juce::Colours::darkgrey);
    g.setFont (18.0f);

    // Header title
    g.setColour (juce::Colours::white);
    g.drawText ("Voice Allocation Monitor", 20, 20, 300, 30, juce::Justification::left);

    // Draw the state of each of our voices
    for (int voice = 0; voice < 5; ++voice)
    {
        int noteNumber = localMidiNotes[voice];
        juce::String voiceDisplayString = "Voice " + juce::String (voice) + ": ";

        if (noteNumber == -1)
        {
            voiceDisplayString += "EMPTY (Idle)";
            g.setColour (juce::Colours::lightgrey);
        }
        else
        {
            voiceDisplayString += "MIDI Note " + juce::String (noteNumber);
            g.setColour (juce::Colours::greenyellow); // Light up active voices!
        }

        // Space out the text readouts vertically
        int yPosition = 70 + (voice * 40);
        g.drawText (voiceDisplayString, 40, yPosition, 300, 30, juce::Justification::left);
    }
}

void AudioPluginAudioProcessorEditor::resized() {}

