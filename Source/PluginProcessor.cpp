#include "PluginProcessor.h"
#include "ResonatorVoice.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout
AudioPluginAudioProcessor::createParameterLayout()

{
    return Parameters::configureParameters();
}

AudioPluginAudioProcessor::AudioPluginAudioProcessor()

    : AudioProcessor(
          BusesProperties()

              .withInput("Input", juce::AudioChannelSet::stereo(), true)

              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),

      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())

{

  for (int i = 0; i < numVoices; ++i)

  {

    synth.addVoice(new ResonatorVoice());
  }

  synth.addSound(new ResonatorSound());
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor() {}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const

{

  if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())

    return false;

  if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono() &&

      layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())

    return false;

  return true;
}

void AudioPluginAudioProcessor::prepareToPlay(double sampleRate,
                                              int samplesPerBlock)

{

  feedback = apvts.getRawParameterValue("FEEDBACK");
  damping = apvts.getRawParameterValue("DAMPING");
  mix = apvts.getRawParameterValue("MIX");
  wetGainDB = apvts.getRawParameterValue("WET_GAIN");

  trigReleaseMS = apvts.getRawParameterValue("TRIG_RELEASE");
  trigThreshDB = apvts.getRawParameterValue("TRIG_THRESHOLD");
  trigSoftness = apvts.getRawParameterValue("TRIG_SOFTNESS");

  envAttackS = apvts.getRawParameterValue("ENV_ATTACK");
  envDecayS = apvts.getRawParameterValue("ENV_DECAY");
  envSustainLinear = apvts.getRawParameterValue("ENV_SUSTAIN");
  envReleaseS = apvts.getRawParameterValue("ENV_RELEASE");

  arpRateIndex = apvts.getRawParameterValue("ARP_RATE");
  arpGateParam = apvts.getRawParameterValue ("ARP_GATE");
  arpModeParam = apvts.getRawParameterValue("ARP_MODE");
  arpScatter = apvts.getRawParameterValue("ARP_SCATTER");

  juce::dsp::ProcessSpec spec;

  spec.sampleRate = sampleRate;

  spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);

  spec.numChannels = 1;

  // Loop through the manager's registered voices to prepare them

  for (int i = 0; i < synth.getNumVoices(); ++i)

  {

    if (auto *voice = dynamic_cast<ResonatorVoice *>(synth.getVoice(i)))

    {

      voice->prepare(spec);
    }
  }

  arp.prepare(sampleRate);
}

void AudioPluginAudioProcessor::releaseResources() {}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages)

{

  juce::ScopedNoDenormals noDenormals;

  auto totalNumInputChannels = getTotalNumInputChannels();

  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {

    buffer.clear(i, 0, buffer.getNumSamples());
  }

  double sampleRate = getSampleRate();
  float currentFeedback = feedback->load();
  float currentDamping = damping->load();
  float currentMix = mix->load() / 100.0f;
  float wetGainLinear = juce::Decibels::decibelsToGain(wetGainDB->load());

  // 2. Prepare & Snapshot Voice ADSR Structural Parameters

  CustomADSR::Parameters envParams;

  envParams.attack = envAttackS->load();
  envParams.decay = envDecayS->load();
  envParams.sustain = envSustainLinear->load();
  envParams.release = envReleaseS->load();

  // Broadcast filter and envelope configurations down to all synth voices

  for (int i = 0; i < synth.getNumVoices(); ++i)
  {
    if (auto *voice = dynamic_cast<ResonatorVoice *>(synth.getVoice(i)))
    {
      voice->updateParameters(currentFeedback, currentDamping, envParams);
    }
  }

  float attackTau = 0.00001f;
  float attackCoef = std::exp(-1.0f / (sampleRate * attackTau));
  float releaseTau = trigReleaseMS->load() / 1000.0f;
  float releaseCoef = std::exp(-1.0f / (sampleRate * releaseTau));

  // 3. Convert threshold dB to a linear ratio multiplier

  float thresholdLinear = juce::Decibels::decibelsToGain(trigThreshDB->load());
  float currentSoftness = trigSoftness->load();

  // Safe read snapshot of original block

  juce::AudioBuffer<float> dryCopy;

  dryCopy.makeCopyOf(buffer);

  // Run arpeggiator on the midi buffer before processing
  // 1. Snapshot your UI parameters from your APVTS layout
    // (Replace these with your actual parameter lookup variables!)

    float gateLength = arpGateParam->load();
    Arpeggiator::ArpMode mode = Arpeggiator::modeFromIndex(static_cast<int>(arpModeParam->load()));   

    
    float scatter = arpScatter->load();

    float subdivisionInBeats = 0.25f; // Default fallback (1/16th)
    switch (static_cast<int>(arpRateIndex->load()))
    {
        case 0: subdivisionInBeats = 1.0f;       break; // 1/4 note
        case 1: subdivisionInBeats = 2.0f / 3.0f; break; // 1/4 triplet
        case 2: subdivisionInBeats = 0.5f;       break; // 1/8 note
        case 3: subdivisionInBeats = 1.0f / 3.0f; break; // 1/8 triplet
        case 4: subdivisionInBeats = 0.25f;      break; // 1/16 note
        case 5: subdivisionInBeats = 1.0f / 6.0f; break; // 1/16 triplet
        case 6: subdivisionInBeats = 0.125f;     break; // 1/32 note
    }


    
    // 2. Push the UI choices directly into your module
    arp.updateSettings (subdivisionInBeats, gateLength, mode, scatter);

    // 3. INTERCEPT: Pass the midiMessages and DAW playhead into the arpeggiator.
    // This swallows your held chords and replaces them with sequential, overlapping notes.
    arp.processMidiBlock (midiMessages, getPlayHead());

  // Create a dynamic iterator to read MIDI timestamps sample-by-sample

  auto midiIterator = midiMessages.begin();

  auto midiEnd = midiMessages.end();

  for (int sample = 0; sample < buffer.getNumSamples(); ++sample)

  {

    // 1. Process any MIDI events occurring precisely at this sample index

    while (midiIterator != midiEnd && (*midiIterator).samplePosition == sample)

    {

      auto message = (*midiIterator).getMessage();

      if (message.isNoteOn())

      {

        // Tell the synth manager to find a free voice and start it

        synth.noteOn(message.getChannel(), message.getNoteNumber(),
                     message.getFloatVelocity());

      }

      else if (message.isNoteOff())

      {

        // Tell the synth manager to release the active voice

        synth.noteOff(message.getChannel(), message.getNoteNumber(),
                      message.getFloatVelocity(), true);

      }

      else if (message.isAllNotesOff())

      {

        synth.allNotesOff(false, true);
      }

      midiIterator++;
    }

    // 2. Calculate your tracking envelope values as normal

    float inputL = dryCopy.getSample(0, sample);

    float inputR =
        (totalNumInputChannels > 1) ? dryCopy.getSample(1, sample) : inputL;

    float absInput = std::abs(inputL);

    // [Your existing transient follower envelope calculations here...]

    envFast = (absInput > envFast)
                  ? (attackCoef * envFast) + ((1.0f - attackCoef) * absInput)

                  : (releaseCoef * envFast) + ((1.0f - releaseCoef) * absInput);

    envSlow = (releaseCoef * envSlow) + ((1.0f - releaseCoef) * absInput);

    float smoothCoef =
        currentSoftness <= 0.0f
            ? 0.0f
            : std::exp(-1.0f / (getSampleRate() * (currentSoftness / 1000.0f)));

    float punchAmount = envFast - (envSlow * thresholdLinear);

    float rawGate = juce::jlimit(0.0f, 1.0f, punchAmount * 10.0f);

    // 2. Run the raw gate through a low-pass smoothing filter

    // This slows down the attack and release time of the gate itself!

    smoothedGate =
        (smoothCoef * smoothedGate) + ((1.0f - smoothCoef) * rawGate);

    // 3. Apply the perfectly smoothed envelope

    float excitationL = inputL * smoothedGate;

    float excitationR = inputR * smoothedGate;

    // 3. Accumulate wet outputs from all currently playing/ringing synthesizer
    // voices

    float summedWetL = 0.0f;

    float summedWetR = 0.0f;

    for (int i = 0; i < synth.getNumVoices(); ++i)

    {

      if (auto *voice = dynamic_cast<ResonatorVoice *>(synth.getVoice(i)))

      {

        if (voice->isVoiceActive())

        {

          voice->processExcitation(excitationL, excitationR, summedWetL,
                                   summedWetR);
        }
      }
    }

    // 4. Mix blend and output assignment

    float scaleFactor = 1.0f / static_cast<float>(synth.getNumVoices());

    float finalWetL = summedWetL * scaleFactor * wetGainLinear;

    float finalWetR = summedWetR * scaleFactor * wetGainLinear;

    float blendedL = (inputL * (1.0f - currentMix)) + (finalWetL * currentMix);

    float blendedR = (inputR * (1.0f - currentMix)) + (finalWetR * currentMix);

    buffer.setSample(0, sample, blendedL);

    if (totalNumInputChannels > 1) {

      buffer.setSample(1, sample, blendedR);
    }
  }

  midiMessages.clear();
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor()

{

  return new AudioPluginAudioProcessorEditor(*this);
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()

{

  return new AudioPluginAudioProcessor();
}

inline float AudioPluginAudioProcessor::midiToHz(int midiNote) {

  return 440.0f *
         std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);
}

float AudioPluginAudioProcessor::calculateCoef(float timeMs, double sampleRate)

{

  if (timeMs <= 0.0f)
    return 0.0f;

  return std::exp(-1.0f /
                  (static_cast<float>(sampleRate) * (timeMs / 1000.0f)));
}
