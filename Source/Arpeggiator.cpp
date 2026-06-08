// Source/Arpeggiator.cpp
#include "Arpeggiator.h"
#include <algorithm>
#include <cstddef>

void Arpeggiator::prepare (double newSampleRate)
{
    // Reserved for future sample-accurate event placement
    juce::ignoreUnused (newSampleRate);
    reset();
}

void Arpeggiator::reset()
{
    heldNotes.clear();
    activeNotes.clear();
    expandedNotes.clear();
    lastPPQ           = -1.0;
    nextTargetPPQ     = -1.0;
    pendingStepLength = -1.0;
    nextStepIndex     = 0;
    poolIndex         = 0;
}

Arpeggiator::ArpMode Arpeggiator::modeFromIndex (int index)
{
    if (index >= 0 && index < static_cast<int>(Arpeggiator::ArpMode::Count))
        return static_cast<Arpeggiator::ArpMode>(index);

    jassertfalse;
    return Arpeggiator::ArpMode::Up;
}

void Arpeggiator::updateSettings (double subdivision, float gateLength, ArpMode mode, float scatter, int newOctaveRange)
{
    if (std::abs (subdivision - stepLengthInBeats) > 1e-9)
        pendingStepLength = subdivision;

    if (newOctaveRange != octaveRange)
    {
        octaveRange = newOctaveRange;
        rebuildExpandedNotes();
    }

    gateLengthPercent = gateLength;
    currentMode       = mode;
    currentScatter    = scatter;
}

//==============================================================================
void Arpeggiator::scheduleNextTrigger()
{
    double gridBoundaryPPQ = nextStepIndex * stepLengthInBeats;
    double maxScatterBeats = stepLengthInBeats * currentScatter;
    double scatterOffset   = randomEngine.nextDouble() * maxScatterBeats;
    nextTargetPPQ          = gridBoundaryPPQ + scatterOffset;
}

void Arpeggiator::processMidiBlock (juce::MidiBuffer& midiMessages, juce::AudioPlayHead* playHead)
{

    if (playHead == nullptr)
        return;

    auto positionInfo = playHead->getPosition();
    if (! positionInfo.hasValue())
        return;

    auto optionalPpq = positionInfo->getPpqPosition();
    if (! optionalPpq.hasValue())
        return;

    double currentPPQ = *optionalPpq;
    bool   isPlaying  = positionInfo->getIsPlaying();

    if (! isPlaying)
    {
        // Transport stopped: pass MIDI through and reset sequencing state
        releaseAllActiveNotes (midiMessages);
        updateHeldNotes (midiMessages);
        lastPPQ           = -1.0;
        nextTargetPPQ     = -1.0;
        pendingStepLength = -1.0;
        return;
    }

    // Detect a DAW loop / rewind
    if (lastPPQ >= 0.0 && currentPPQ < lastPPQ)
    {
        releaseAllActiveNotes (midiMessages);
        nextTargetPPQ = -1.0;
    }

    lastPPQ = currentPPQ;

    // Apply a pending rate change now that we have currentPPQ. Recompute
    // nextStepIndex and nextTargetPPQ from scratch against the new step length
    // so the target always lands in the near future regardless of direction.
    if (pendingStepLength > 0.0)
    {
        stepLengthInBeats = pendingStepLength;
        pendingStepLength = -1.0;

        nextStepIndex = static_cast<int> (std::floor (currentPPQ / stepLengthInBeats)) + 1;
        scheduleNextTrigger();
    }

    // Update heldNotes from incoming MIDI, stripping note events from the
    // output buffer so they don't bleed through as raw notes while playing.
    consumeIncomingMidi (midiMessages);

    if (heldNotes.empty())
    {
        nextTargetPPQ = -1.0;
        checkScheduledNoteOffs (midiMessages, currentPPQ);
        return;
    }

    // On the very first block (or after a reset), schedule the first trigger
    if (nextTargetPPQ < 0.0)
    {
        nextStepIndex = static_cast<int> (std::floor (currentPPQ / stepLengthInBeats)) + 1;
        scheduleNextTrigger();
    }

    // Edge-trigger: has the playhead crossed the scattered target time?
    if (currentPPQ >= nextTargetPPQ)
    {
        triggerNextNote (midiMessages, currentPPQ);

        nextStepIndex++;
        scheduleNextTrigger();
    }

    checkScheduledNoteOffs (midiMessages, currentPPQ);
}

//==============================================================================
void Arpeggiator::updateHeldNotes (const juce::MidiBuffer& incomingMidi)
{
    for (const auto metadata : incomingMidi)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            int note = message.getNoteNumber();

            if (std::find (heldNotes.begin(), heldNotes.end(), note) == heldNotes.end())
                heldNotes.push_back (note);
        }
        else if (message.isNoteOff())
        {
            int note = message.getNoteNumber();
            auto it  = std::find (heldNotes.begin(), heldNotes.end(), note);

            if (it != heldNotes.end())
                heldNotes.erase (it);
        }
    }

    // Keep sorted lowest-to-highest so Up/Down modes have a predictable base
    std::sort (heldNotes.begin(), heldNotes.end());

    rebuildExpandedNotes();
}

void Arpeggiator::consumeIncomingMidi (juce::MidiBuffer& incomingMidi)
{
    juce::MidiBuffer filteredMidi;

    for (const auto metadata : incomingMidi)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            int note = message.getNoteNumber();

            if (std::find (heldNotes.begin(), heldNotes.end(), note) == heldNotes.end())
                heldNotes.push_back (note);
        }
        else if (message.isNoteOff())
        {
            int note = message.getNoteNumber();
            auto it  = std::find (heldNotes.begin(), heldNotes.end(), note);

            if (it != heldNotes.end())
                heldNotes.erase (it);
        }
        else
        {
            // Non-note events (CC, pitch bend, etc.) always pass through
            filteredMidi.addEvent (message, metadata.samplePosition);
        }
    }

    incomingMidi.swapWith (filteredMidi);

    // Keep sorted lowest-to-highest so Up/Down modes have a predictable base
    std::sort (heldNotes.begin(), heldNotes.end());

    rebuildExpandedNotes();
}

void Arpeggiator::rebuildExpandedNotes()
{
    expandedNotes.clear();

    if (heldNotes.empty())
        return;

    if (octaveRange == 0)
    {
        expandedNotes = heldNotes;
        return;
    }

    int low  = std::min (0, octaveRange);
    int high = std::max (0, octaveRange);

    for (int octave = low; octave <= high; ++octave)
    {
        for (int note : heldNotes)
        {
            int shifted = note + (octave * 12);

            if (shifted >= 0 && shifted <= 127)
                expandedNotes.push_back (shifted);
        }
    }

    // Keep sorted so Up/Down modes have a predictable base
    std::sort (expandedNotes.begin(), expandedNotes.end());
}

int Arpeggiator::selectNextNote()
{
    const size_t size = expandedNotes.size();

    switch (currentMode)
    {
        case ArpMode::Down:
        {
            if (poolIndex < 0 || poolIndex >= size)
                poolIndex = size - 1;

            return expandedNotes[poolIndex--];
        }

        case ArpMode::Updown:
        {
            if (size == 1)
                return expandedNotes[0];

            poolIndex = std::clamp(poolIndex, (size_t)0, size-1);
            int note  = expandedNotes[poolIndex];

            if (arpGoingUp)
            {
                if (poolIndex >= size - 1)
                {
                    arpGoingUp = false;
                    poolIndex--;
                }
                else
                {
                    poolIndex++;
                }
            }
            else
            {
                if (poolIndex <= 0)
                {
                    arpGoingUp = true;
                    poolIndex++;
                }
                else
                {
                    poolIndex--;
                }
            }

            return note;
        }

        case ArpMode::Random:
            return expandedNotes[randomEngine.nextInt (size)];

        case ArpMode::Up:
        default:
        {
            if (poolIndex < 0 || poolIndex >= size)
                poolIndex = 0;

            return expandedNotes[poolIndex++];
        }
    }
}

void Arpeggiator::triggerNextNote (juce::MidiBuffer& outputMidi, double currentPPQ)
{
    if (heldNotes.empty())
        return;

    int noteToPlay = selectNextNote();
    
    // Clamp to valid MIDI range
    noteToPlay = juce::jlimit (0, 127, noteToPlay);

    int velocity = 90;
    double offPPQ = currentPPQ + (stepLengthInBeats * gateLengthPercent);

    for (int i = static_cast<int> (activeNotes.size()) - 1; i >= 0; --i)
    {
        if (activeNotes[i].midiNoteNumber == noteToPlay)
        {
            outputMidi.addEvent (juce::MidiMessage::noteOff (1, noteToPlay), 0);
            activeNotes.erase (activeNotes.begin() + i);
        }
    }

    activeNotes.push_back ({ noteToPlay, velocity, offPPQ });
    outputMidi.addEvent (juce::MidiMessage::noteOn (1, noteToPlay,
                                                     static_cast<juce::uint8> (velocity)), 0);
}

void Arpeggiator::checkScheduledNoteOffs (juce::MidiBuffer& outputMidi, double currentPPQ)
{
    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        size_t it = static_cast<size_t>(i);
        if (currentPPQ >= activeNotes[it].targetOffPPQ)
        {
            outputMidi.addEvent (juce::MidiMessage::noteOff (1, activeNotes[it].midiNoteNumber), 0);
            activeNotes.erase (activeNotes.begin() + static_cast<int>(i));
        }
    }
}

void Arpeggiator::releaseAllActiveNotes (juce::MidiBuffer& outputMidi)
{
    for (const auto& note : activeNotes) {
        outputMidi.addEvent (juce::MidiMessage::noteOff (1, note.midiNoteNumber), 0);
    }
    activeNotes.clear();
}