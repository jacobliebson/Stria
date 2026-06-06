// Source/Arpeggiator.cpp
#include "Arpeggiator.h"
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

void Arpeggiator::updateSettings (double subdivision, float gateLength, ArpMode mode, float scatter)
{
    // Rate changes are applied in processMidiBlock where currentPPQ is available,
    // so we can reschedule nextTargetPPQ correctly from the actual playhead position.
    
    if (std::abs(subdivision - stepLengthInBeats) > 1e-9) // Use epsilon tolerance for safety
        pendingStepLength = subdivision;

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
}

int Arpeggiator::selectNextNote()
{
    const size_t size = heldNotes.size();

    switch (currentMode)
    {
        case ArpMode::Down:
        {
            if (poolIndex < 0 || poolIndex >= size)
                poolIndex = size - 1;

            return heldNotes[poolIndex--];
        }

        case ArpMode::Random:
            return heldNotes[static_cast<size_t>(randomEngine.nextInt (static_cast<int>(size)))];

        case ArpMode::Up:
        default:
        {
            if (poolIndex < 0 || poolIndex >= size)
                poolIndex = 0;

            return heldNotes[poolIndex++];
        }
        case ArpMode::Count:
            return -1;
    }
}

void Arpeggiator::triggerNextNote (juce::MidiBuffer& outputMidi, double currentPPQ)
{
    if (heldNotes.empty())
        return;

    int noteToPlay = selectNextNote();
    int velocity   = 90;

    double offPPQ = currentPPQ + (stepLengthInBeats * gateLengthPercent);

    // If this note is already sounding, retire it before retriggering
    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i)
    {
        if (activeNotes[static_cast<size_t>(i)].midiNoteNumber == noteToPlay)
        {
            outputMidi.addEvent (juce::MidiMessage::noteOff (1, noteToPlay), 0);
            activeNotes.erase (activeNotes.begin() + static_cast<int>(i));
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
    for (const auto& note : activeNotes)
        outputMidi.addEvent (juce::MidiMessage::noteOff (1, note.midiNoteNumber), 0);

    activeNotes.clear();
}