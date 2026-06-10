#include "PluginProcessor.h"
#include "ResonatorVoice.h"
#include "PluginEditor.h"

#include <fstream>
#include <juce_core/juce_core.h>

static std::ofstream gateLogFile;
static int gateLogCounter = 0;

juce::AudioProcessorValueTreeState::ParameterLayout
AudioPluginAudioProcessor::createParameterLayout()
{
    return Parameters::configureParameters();
}

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor (
          BusesProperties()
              .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
              .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < numArpVoices; ++i)
        arpSynth.addVoice (new ResonatorVoice());

    for (int i = 0; i < numChordVoices; ++i)
        chordSynth.addVoice (new ResonatorVoice());

    arpSynth.addSound   (new ResonatorSound());
    chordSynth.addSound (new ResonatorSound());

    auto logPath = juce::File::getSpecialLocation (juce::File::userDesktopDirectory)
                   .getChildFile ("gate_log.csv");
gateLogFile.open (logPath.getFullPathName().toStdString());
gateLogFile << "sample,absInput,thresholdLinear,isAboveThreshold,wasAboveThreshold,state,envelopeVal,holdCounter,gateGain\n";
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
    feedback         = apvts.getRawParameterValue ("FEEDBACK");
    damping          = apvts.getRawParameterValue ("DAMPING");
    detune           = apvts.getRawParameterValue ("DETUNE");
    detuneMode       = apvts.getRawParameterValue ("DETUNE_MODE");


    mix              = apvts.getRawParameterValue ("MIX");
    arpGainDB        = apvts.getRawParameterValue ("ARP_GAIN");
    chordGainDB      = apvts.getRawParameterValue ("CHORD_GAIN");

    
    trigAttack       = apvts.getRawParameterValue ("TRIG_ATTACK");
    trigHold       = apvts.getRawParameterValue ("TRIG_HOLD");
    trigRelease      = apvts.getRawParameterValue ("TRIG_RELEASE");
    trigThreshold    = apvts.getRawParameterValue ("TRIG_THRESHOLD");


    arpEnvAttack     = apvts.getRawParameterValue ("ARP_ENV_ATTACK");
    arpEnvDecay      = apvts.getRawParameterValue ("ARP_ENV_DECAY");
    arpEnvSustain    = apvts.getRawParameterValue ("ARP_ENV_SUSTAIN");
    arpEnvRelease    = apvts.getRawParameterValue ("ARP_ENV_RELEASE");

    chordEnvAttack     = apvts.getRawParameterValue ("CHORD_ENV_ATTACK");
    chordEnvDecay      = apvts.getRawParameterValue ("CHORD_ENV_DECAY");
    chordEnvSustain    = apvts.getRawParameterValue ("CHORD_ENV_SUSTAIN");
    chordEnvRelease    = apvts.getRawParameterValue ("CHORD_ENV_RELEASE");


    arpRateIndex     = apvts.getRawParameterValue ("ARP_RATE");
    arpGateParam     = apvts.getRawParameterValue ("ARP_GATE");
    arpModeParam     = apvts.getRawParameterValue ("ARP_MODE");
    arpScatter       = apvts.getRawParameterValue ("ARP_SCATTER");
    arpDeviation     = apvts.getRawParameterValue ("ARP_DEVIATION");
    octRange         = apvts.getRawParameterValue("ARP_RANGE");

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = 1;

    for (int i = 0; i < arpSynth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<ResonatorVoice*> (arpSynth.getVoice (i)))
            voice->prepare (spec);

    for (int i = 0; i < chordSynth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<ResonatorVoice*> (chordSynth.getVoice (i)))
            voice->prepare (spec);

    arp.prepare (sampleRate);
}

void AudioPluginAudioProcessor::releaseResources() {}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto playHead = getPlayHead();
    bool isPlaying = false;
    double currentPPQ = 0.0;

    if (playHead != nullptr)
    {
        auto position = playHead->getPosition();
        if (position.hasValue())
        {
            isPlaying = position->getIsPlaying();
            if (auto ppq = position->getPpqPosition())
                currentPPQ = *ppq;
        }
    }

    bool rewindDetected = isPlaying && lastProcessorPPQ >= 0.0 && currentPPQ < lastProcessorPPQ;
    bool stoppedPlaying = wasPlaying && !isPlaying;

    if (rewindDetected || stoppedPlaying) {
        arpSynth.allNotesOff (0, true);
        gateValue.store (0.0f, std::memory_order_relaxed);
    }

    wasPlaying        = isPlaying;
    lastProcessorPPQ  = isPlaying ? currentPPQ : -1.0;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Snapshot parameters
    double sampleRate = getSampleRate();

    float currentFeedback    = feedback->load();
    float currentDamping     = damping->load();
    float currentDetune      = detune->load();
    int currentDetuneMode    = (int)detuneMode->load();

    CustomADSR::Parameters gateParams;
    gateParams.attack  = trigAttack->load() / 1000.0f;
    gateParams.decay   = 0.0f;
    gateParams.sustain = 1.0f;
    gateParams.hold    = trigHold->load() / 1000.0f;
    gateParams.release = trigRelease->load() / 1000.0f;
    gateParams.useHoldPhase = true;
    gateEnvelope.setParameters(gateParams);
    float thresholdLinear = juce::Decibels::decibelsToGain (trigThreshold->load());

    float currentMix         = mix->load() / 100.0f;
    float currentArpGainDB   = arpGainDB->load() + 12.0f;
    float currentChordGainDB = chordGainDB->load() + 12.0f;

    static constexpr float silenceThresholdDB = -60.0f;
    float arpGainLinear   = (currentArpGainDB   <= silenceThresholdDB) ? 0.0f : juce::Decibels::decibelsToGain (currentArpGainDB);
    float chordGainLinear = (currentChordGainDB <= silenceThresholdDB )? 0.0f : juce::Decibels::decibelsToGain (currentChordGainDB);

    CustomADSR::Parameters arpEnvParams;
    arpEnvParams.attack  = arpEnvAttack ->load();
    arpEnvParams.decay   = arpEnvDecay  ->load();
    arpEnvParams.sustain = arpEnvSustain->load() / 100.0f;
    arpEnvParams.release = arpEnvRelease->load();
    arpEnvParams.hold    = 0.0f;
    arpEnvParams.useHoldPhase = false;

    CustomADSR::Parameters chordEnvParams;
    chordEnvParams.attack  = chordEnvAttack ->load();
    chordEnvParams.decay   = chordEnvDecay  ->load();
    chordEnvParams.sustain = chordEnvSustain->load() / 100.0f;
    chordEnvParams.release = chordEnvRelease->load();
    chordEnvParams.hold    = 0.0f;
    chordEnvParams.useHoldPhase = false;

    // Broadcast parameters to both voice pools
    // NOTE: When independent envelopes are added, pass VoiceRole here to route
    // different envParams to arpSynth vs chordSynth voices.
    for (int i = 0; i < arpSynth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<ResonatorVoice*> (arpSynth.getVoice (i)))
            voice->updateParameters (currentFeedback, currentDamping, arpEnvParams, currentDetune, currentDetuneMode);

    for (int i = 0; i < chordSynth.getNumVoices(); ++i)
        if (auto* voice = dynamic_cast<ResonatorVoice*> (chordSynth.getVoice (i)))
            voice->updateParameters (currentFeedback, currentDamping, chordEnvParams, currentDetune, currentDetuneMode);

    
    // --- Visualizer: snapshot active notes (once per block) ---
    {
        // Compute linear gains once before the loops
        // Mirrors the existing silenceThresholdDB logic already in processBlock
        const float arpLinear   = (currentArpGainDB   <= silenceThresholdDB) ? 0.0f 
                                : juce::Decibels::decibelsToGain (currentArpGainDB);
        const float chordLinear = (currentChordGainDB <= silenceThresholdDB) ? 0.0f 
                                : juce::Decibels::decibelsToGain (currentChordGainDB);

        int count = 0;

        for (int i = 0; i < arpSynth.getNumVoices() && count < maxActiveNotes; ++i)
        {
            if (auto* voice = dynamic_cast<ResonatorVoice*> (arpSynth.getVoice (i)))
            {
                if (voice->isVoiceActive())
                {
                    activeNoteSnapshot[count].frequency  = voice->getCurrentFrequency();
                    activeNoteSnapshot[count].amplitude = voice->getEnvelopeLevel() * arpLinear;
                    activeNoteSnapshot[count].isArp     = true;
                    ++count;
                }
            }
        }

        for (int i = 0; i < chordSynth.getNumVoices() && count < maxActiveNotes; ++i)
        {
            if (auto* voice = dynamic_cast<ResonatorVoice*> (chordSynth.getVoice (i)))
            {
                if (voice->isVoiceActive())
                {
                    activeNoteSnapshot[count].frequency  = voice->getCurrentFrequency();
                    activeNoteSnapshot[count].amplitude = voice->getEnvelopeLevel() * chordLinear;
                    activeNoteSnapshot[count].isArp     = false;
                    ++count;
                }
            }
        }

        activeNoteCount.store (count, std::memory_order_release);
    }
    // --- end visualizer snapshot ---


    // Safe read snapshot of dry input
    juce::AudioBuffer<float> dryCopy;
    dryCopy.makeCopyOf (buffer);

    // Build the chord MIDI stream: a copy of the raw input, untouched by the arpeggiator.
    // This always reflects what the user is physically holding.
    juce::MidiBuffer chordMidi;
    chordMidi.addEvents (midiMessages, 0, -1, 0);

    // Build the arp MIDI stream: the arpeggiator consumes note events from
    // midiMessages and replaces them with sequential arp notes.
    float gateLength = arpGateParam->load() / 100.0f;
    Arpeggiator::ArpMode mode = Arpeggiator::modeFromIndex (static_cast<int> (arpModeParam->load()));
    float scatter    = arpScatter->load() / 100.0f;
    float deviation  = arpDeviation->load() / 100.0f;
    int range        = static_cast<int>(octRange->load());

    double subdivisionInBeats = 0.25; // Default fallback (1/16th)
    int rateIndex = static_cast<int> (arpRateIndex->load());

    switch (rateIndex)
    {
        case 0: subdivisionInBeats = 1.0;       break; // 1/4 note
        case 1: subdivisionInBeats = 2.0 / 3.0; break; // 1/4 triplet
        case 2: subdivisionInBeats = 0.5;       break; // 1/8 note
        case 3: subdivisionInBeats = 1.0 / 3.0; break; // 1/8 triplet
        case 4: subdivisionInBeats = 0.25;      break; // 1/16 note
        case 5: subdivisionInBeats = 1.0 / 6.0; break; // 1/16 triplet
        case 6: subdivisionInBeats = 0.125;     break; // 1/32 note
        default:                                break;   
    }

    if (rateIndex < 7)
    {
        arp.updateSettings (subdivisionInBeats, gateLength, mode, scatter, deviation, range);
        arp.processMidiBlock (midiMessages, getPlayHead());
         
    }
    // midiMessages now contains arp notes; chordMidi contains raw held notes.
    // Both iterators advance independently through the same sample range below.
    auto arpMidiIt    = midiMessages.begin();
    auto arpMidiEnd   = midiMessages.end();
    auto chordMidiIt  = chordMidi.begin();
    auto chordMidiEnd = chordMidi.end();

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Drive arpSynth from the arpeggiator MIDI stream
        while (arpMidiIt != arpMidiEnd && (*arpMidiIt).samplePosition == sample)
        {
            auto message = (*arpMidiIt).getMessage();

            if (message.isNoteOn())
                arpSynth.noteOn (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
            else if (message.isNoteOff())
                arpSynth.noteOff (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity(), true);
            else if (message.isAllNotesOff())
                arpSynth.allNotesOff (false, true);

            ++arpMidiIt;
        }

        // Drive chordSynth from the raw chord MIDI stream
        while (chordMidiIt != chordMidiEnd && (*chordMidiIt).samplePosition == sample)
        {
            auto message = (*chordMidiIt).getMessage();

            if (message.isNoteOn())
                chordSynth.noteOn (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
            else if (message.isNoteOff())
                chordSynth.noteOff (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity(), true);
            else if (message.isAllNotesOff())
                chordSynth.allNotesOff (false, true);

            ++chordMidiIt;
        }


        float inputL   = dryCopy.getSample (0, sample);
        float inputR   = (totalNumInputChannels > 1) ? dryCopy.getSample (1, sample) : inputL;
        float absInput = std::abs (inputL);


        bool isAboveThreshold = absInput > thresholdLinear;
        
        if (isAboveThreshold && !wasAboveThreshold && !gateEnvelope.isActive())
            gateEnvelope.noteOn();
     
        wasAboveThreshold = isAboveThreshold;

        constexpr float minThresh = 0.0011f;
        float gateGain = thresholdLinear < minThresh? 1.0f : gateEnvelope.getNextSample();

        // --- TEMP LOGGING ---
        if (gateLogFile.is_open() && gateLogCounter < 200000) // cap to avoid huge files
        {
            gateLogFile << gateLogCounter << ","
                        << absInput << ","
                        << thresholdLinear << ","
                        << (isAboveThreshold ? 1 : 0) << ","
                        << (wasAboveThreshold ? 1 : 0) << ","
                        << gateEnvelope.getStateInt() << ","
                        << gateEnvelope.getEnvLevel() << ","
                        << gateEnvelope.getHoldCounter() << ","
                        << gateGain << "\n";
            ++gateLogCounter;
        }
        // --- END TEMP LOGGING ---

        float excitationL = inputL * gateGain;
        float excitationR = inputR * gateGain;


        // --- Visualizer: decimated noise ring buffer ---
        if (++noiseDecimationCounter >= noiseDecimationFactor)
        {
            noiseDecimationCounter = 0;
            int pos = noiseWritePos.load (std::memory_order_relaxed);
            noiseRingBuffer[pos].store (absInput, std::memory_order_relaxed);
            noiseWritePos.store ((pos + 1) % noiseRingSize, std::memory_order_release);
            gateValue.store (gateGain, std::memory_order_relaxed);
        }
        // --- end ring buffer write ---
        
        
        // Accumulate wet output from both voice pools
        float summedArpL = 0.0f;
        float summedArpR = 0.0f;

        for (int i = 0; i < arpSynth.getNumVoices(); ++i)
            if (auto* voice = dynamic_cast<ResonatorVoice*> (arpSynth.getVoice (i)))
                if (voice->isVoiceActive())
                    voice->processExcitation (excitationL, excitationR, summedArpL, summedArpR);
   
        float summedChordsL = 0.0f;
        float summedChordsR = 0.0f;

        for (int i = 0; i < chordSynth.getNumVoices(); ++i)
            if (auto* voice = dynamic_cast<ResonatorVoice*> (chordSynth.getVoice (i)))
                if (voice->isVoiceActive())
                    voice->processExcitation (excitationL, excitationR, summedChordsL, summedChordsR);

        // Scale each by voice count so summed output stays at a consistent level
        float scaleFactorArp = 1.0f / static_cast<float> (arpSynth.getNumVoices());
        float finalArpL = summedArpL * scaleFactorArp * arpGainLinear;
        float finalArpR = summedArpR * scaleFactorArp * arpGainLinear;

        float scaleFactorChords = 1.0f / static_cast<float> (chordSynth.getNumVoices());
        float finalChordsL = summedChordsL * scaleFactorChords * chordGainLinear;
        float finalChordsR = summedChordsR * scaleFactorChords * chordGainLinear;

        float finalWetL = finalArpL + finalChordsL;
        float finalWetR = finalArpR + finalChordsR;
        
        float blendedL = (inputL * (1.0f - currentMix)) + (finalWetL * currentMix);
        float blendedR = (inputR * (1.0f - currentMix)) + (finalWetR * currentMix);

        buffer.setSample (0, sample, blendedL);

        if (totalNumInputChannels > 1)
            buffer.setSample (1, sample, blendedR);
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

inline float AudioPluginAudioProcessor::midiToHz (int midiNote)
{
    return 440.0f * std::pow (2.0f, (static_cast<float> (midiNote) - 69.0f) / 12.0f);
}

float AudioPluginAudioProcessor::calculateCoef (float timeMs, double sampleRate)
{
    if (timeMs <= 0.0f)
        return 0.0f;

    return std::exp (-1.0f / (static_cast<float> (sampleRate) * (timeMs / 1000.0f)));
}