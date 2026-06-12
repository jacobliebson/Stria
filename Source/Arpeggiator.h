// Source/Arpeggiator.h
#pragma once

#include <cstddef>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <algorithm>

class Arpeggiator
{
public:
    enum class ArpMode { Up, Down, Updown, Count };
    static ArpMode modeFromIndex (int index);

    Arpeggiator() = default;
    ~Arpeggiator() = default;

    void reset();

    // Called during your plugin's prepareToPlay
    void prepare (double newSampleRate);
    void forceStop(juce::MidiBuffer& outputMidi);

    // Updates the settings from your UI sliders/menus
    void updateSettings (double subdivision, float gateLength, ArpMode mode, float scatter, float deviation, int newOctaveRange);

    // The main processing pipeline called at the top of PluginProcessor::processBlock
    void processMidiBlock (juce::MidiBuffer& midiMessages, juce::AudioPlayHead* playHead, bool sustainActive);


private:
    struct ActiveNote
    {
        int    midiNoteNumber;
        int    velocity;
        double targetOffPPQ;
    };

    // Schedules the next trigger time relative to nextStepIndex and the current grid
    void scheduleNextTrigger();

    // Updates heldNotes from all note-on/off events in the buffer
    void updateHeldNotes (const juce::MidiBuffer& incomingMidi);

    // Reads note events into heldNotes and rebuilds the buffer with them stripped,
    // so they don't bleed through to the output as raw notes
    void consumeIncomingMidi (juce::MidiBuffer& incomingMidi, bool sustainActive);

    // Expands active notes to the specified octave range
    void rebuildExpandedNotes();

    void triggerNextNote (juce::MidiBuffer& outputMidi, double currentPPQ);
    void checkScheduledNoteOffs (juce::MidiBuffer& outputMidi, double currentPPQ);
    void releaseAllActiveNotes (juce::MidiBuffer& outputMidi);

    void flushSustainedNotes();
    

    int selectNextNote();

    // Step settings
    double  stepLengthInBeats = 0.25;          // Default: 1/16th note
    float   gateLengthPercent = 0.8f;
    ArpMode currentMode       = ArpMode::Up;
    float   currentScatter    = 0.0f;
    float   currentDeviation  = 0.0f;

    // Octave range 
    int  octaveRange   = 0;
    std::vector<int> expandedNotes; 

    // Transport / sequencing state
    double lastPPQ           = -1.0;
    double nextTargetPPQ     = -1.0;  // Scattered trigger time for the upcoming step
    double pendingStepLength = -1.0;  // Rate change staged by updateSettings, applied in processMidiBlock
    int    nextStepIndex     = 0;     // Grid step index of the next note to fire
    size_t    poolIndex      = 0;     // Position within heldNotes for Up/Down modes
    bool arpGoingUp             = false; 


    juce::Random randomEngine;

    bool wasSustainActive = false;

    // Note pools
    std::vector<int>        heldNotes;   // MIDI note numbers of physically held keys
    std::vector<int>        sustainedNotes;
    std::vector<ActiveNote> activeNotes; // Notes currently sounding
};