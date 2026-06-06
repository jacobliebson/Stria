// Source/Arpeggiator.cpp
#include "Arpeggiator.h"

void Arpeggiator::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void Arpeggiator::reset()
{
    lastStepIndex = -1;
    currentPoolIndex = 0;
    heldNotes.clear();
    activeNotes.clear();
}

void Arpeggiator::updateSettings (float subdivision, float gateLength, int mode)
{
    // Convert musical subdivision (e.g., 0.25 for 1/16th note) to beats
    stepLengthInBeats = subdivision; 
    gateLengthPercent = gateLength;
    currentMode = mode;
}

void Arpeggiator::processMidiBlock (juce::MidiBuffer& midiMessages, 
                                     juce::AudioPlayHead* playHead, 
                                     int numSamples)
{
    handleIncomingMidi (midiMessages);

    juce::MidiBuffer incomingMidiCopy = midiMessages; 
    midiMessages.clear();

    if (playHead == nullptr)
    {
        lastStepIndex = -1; 
        lastPPQ = -1.0; // Reset tracking here too
        return;
    }

    auto positionInfo = playHead->getPosition();
    if (positionInfo.hasValue())
    {
        auto optionalPpq = positionInfo->getPpqPosition();
        bool isPlaying = positionInfo->getIsPlaying();

        if (optionalPpq.hasValue())
        {
            double currentPPQ = *optionalPpq;

            // --- DAW LOOP DETECTION CATCH ---
            // If the current time is suddenly less than our last recorded time,
            // the user has either hit a loop point or jumped the timeline cursor backwards.
            if (isPlaying && lastPPQ >= 0.0 && currentPPQ < lastPPQ)
            {
                // Forcefully kill all currently sustaining virtual notes immediately
                for (const auto& note : activeNotes)
                {
                    midiMessages.addEvent (juce::MidiMessage::noteOff (1, note.midiNoteNumber), 0);
                }
                activeNotes.clear();
                lastStepIndex = -1; // Reset step tracking so it catches the next grid line correctly
            }
            
            // Store the current timeline position for comparison on the next block
            lastPPQ = currentPPQ;

// Only step forward if the transport timeline is actively rolling
            if (isPlaying)
            {
                // TRANSITION DETECTED: The user just hit Play!
                if (!wasPlayingLastBlock)
                {
                    wasPlayingLastBlock = true;
                    
                    // ONLY reset timing states. Do NOT clear or alter the MIDI buffer here!
                    lastStepIndex = -1; 
                    lastPPQ = currentPPQ;
                    activeNotes.clear();
                }

                // 1. Parse the incoming keys to maintain our sorted chord pool uniformly
                handleIncomingMidi (midiMessages);

                // 2. Clear out the original incoming MIDI buffer completely
                midiMessages.clear();

                if (heldNotes.empty())
                {
                    lastStepIndex = -1;
                }
                else
                {
                    int currentStepIndex = std::floor (currentPPQ / stepLengthInBeats);

                    if (currentStepIndex != lastStepIndex)
                    {
                        lastStepIndex = currentStepIndex;
                        checkAndTriggerNewSteps (midiMessages, currentPPQ);
                    }
                }
            }
            else
            {
                wasPlayingLastBlock = false;
                lastStepIndex = -1;
                lastPPQ = -1.0;
            }

            checkScheduledNoteOffs (midiMessages, currentPPQ);
        }
    }
}

void Arpeggiator::handleIncomingMidi (juce::MidiBuffer& incomingMidi)
{
    // Listen to the incoming MIDI messages to maintain our chord pool
    for (const auto metadata : incomingMidi)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            int noteNumber = message.getNoteNumber();
            
            // Only add the note if it isn't already in our held pool
            if (std::find (heldNotes.begin(), heldNotes.end(), noteNumber) == heldNotes.end())
            {
                heldNotes.push_back (noteNumber);
            }
        }
        else if (message.isNoteOff())
        {
            int noteNumber = message.getNoteNumber();
            
            // Remove the note from our pool when the user lifts their finger
            auto it = std::find (heldNotes.begin(), heldNotes.end(), noteNumber);
            if (it != heldNotes.end())
            {
                heldNotes.erase (it);
            }
        }
    }

    // Always keep the note array sorted lowest-to-highest pitch 
    // so our Up/Down sorting algorithms have a predictable base.
    std::sort (heldNotes.begin(), heldNotes.end());
}

void Arpeggiator::checkAndTriggerNewSteps (juce::MidiBuffer& outputMidi, double currentPPQ)
{
    if (heldNotes.empty()) return;

    // Wrap the pool index back around if it exceeds our held chord size
    if (currentPoolIndex >= static_cast<int> (heldNotes.size()))
    {
        currentPoolIndex = 0;
    }

    // Pick our note from our sorted collection
    int noteToPlay = heldNotes[currentPoolIndex];
    int velocity = 90; // Default nominal velocity

    // Calculate exactly when this note needs to turn off based on Gate Length
    // e.g., if step length is 0.25 beats and gate is 150% (1.5), it lasts 0.375 beats.
    double durationInBeats = stepLengthInBeats * gateLengthPercent;
    double offPPQ = currentPPQ + durationInBeats;

    // 1. Add this note to our active tracking queue so we can kill it later
    activeNotes.push_back ({ noteToPlay, velocity, offPPQ });

    // 2. Inject the Note On event into our output buffer at sample 0 (instant)
    outputMidi.addEvent (juce::MidiMessage::noteOn (1, noteToPlay, static_cast<juce::uint8> (velocity)), 0);

    // 3. Advance our sequence counter to the next note for the next clock tick!
    currentPoolIndex++;
}

void Arpeggiator::checkScheduledNoteOffs (juce::MidiBuffer& outputMidi, double currentPPQ)
{
    // Loop backwards through our active notes vector so we can safely erase items
    for (int i = static_cast<int> (activeNotes.size()) - 1; i >= 0; --i)
    {
        // Has the DAW timeline passed this specific note's expiration mark?
        if (currentPPQ >= activeNotes[i].targetOffPPQ)
        {
            // Inject the Note Off message into the output buffer at sample 0
            outputMidi.addEvent (juce::MidiMessage::noteOff (1, activeNotes[i].midiNoteNumber), 0);
            
            // Remove it from our active tracker
            activeNotes.erase (activeNotes.begin() + i);
        }
    }
}