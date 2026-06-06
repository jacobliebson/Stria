// Source/Arpeggiator.h
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <algorithm>

class Arpeggiator
{
public:
    Arpeggiator() = default;
    ~Arpeggiator() = default;

    void reset();

    // Called during your plugin's prepareToPlay
    void prepare (double sampleRate);
    
    // Updates the settings from your UI sliders/menus
    void updateSettings (float subdivision, float gateLength, int mode);

    // The main processing pipeline called at the top of PluginProcessor::processBlock
    void processMidiBlock (juce::MidiBuffer& midiMessages, 
                           juce::AudioPlayHead* playHead, 
                           int numSamples);

private:
    struct ActiveNote
    {
        int midiNoteNumber;
        int velocity;
        double targetOffPPQ;
    };

    // Helper functions to keep code modular
    void handleIncomingMidi (juce::MidiBuffer& incomingMidi);
    void checkAndTriggerNewSteps (juce::MidiBuffer& outputMidi, double currentPPQ);
    void checkScheduledNoteOffs (juce::MidiBuffer& outputMidi, double currentPPQ);

    // Timing tracking
    double sampleRate = 44100.0;
    double stepLengthInBeats = 0.25; // Default to 1/16th notes (0.25 of a quarter note)
    float gateLengthPercent = 0.8f;   // 80% of step length
    int currentMode = 0;              // 0 = Up, 1 = Down, etc.
    bool wasPlayingLastBlock = false;

    int lastStepIndex = -1;
    int currentPoolIndex = 0;
    double lastPPQ = -1.0;

    // Note storage collections
    std::vector<int> heldNotes;          // The physical keys currently down
    std::vector<ActiveNote> activeNotes; // The virtual keys currently ringing out
};