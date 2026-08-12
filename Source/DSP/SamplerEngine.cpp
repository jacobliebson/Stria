// Source/DSP/SamplerEngine.cpp
#include "SamplerEngine.h"

SamplerEngine::SamplerEngine() {}

SamplerEngine::~SamplerEngine()
{
    // Ensure we don't delete a buffer the audio thread is reading.
    // In practice the processor will have been stopped before destruction.
    auto* buf = activeBuffer.exchange (nullptr);
    delete buf;
}




//==============================================================================
// Message thread
//==============================================================================

bool SamplerEngine::loadFile (const juce::File& file, double targetSampleRate)
{
    startPoint = 0.0f;
    endPoint = 1.0f;
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr)
        return false;

    // Decode into a staging buffer at the target sample rate
    const int numChannels = juce::jmin (2, static_cast<int> (reader->numChannels));
    const int numSamples  = static_cast<int> (reader->lengthInSamples);

    stagingBuffer = std::make_unique<juce::AudioBuffer<float>> (numChannels, numSamples);
    reader->read (stagingBuffer.get(), 0, numSamples, 0, true, true);

    // Resample if needed
    if (reader->sampleRate != targetSampleRate && reader->sampleRate > 0)
    {
        const double ratio      = targetSampleRate / reader->sampleRate;
        const int    newSamples = static_cast<int> (numSamples * ratio);

        auto resampled = std::make_unique<juce::AudioBuffer<float>> (numChannels, newSamples);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* src = stagingBuffer->getReadPointer (ch);
            float*       dst = resampled->getWritePointer (ch);

            for (int i = 0; i < newSamples; ++i)
            {
                const double srcPos = static_cast<double> (i) / ratio;
                dst[i] = interpolate (src, numSamples, srcPos);
            }
        }

        stagingBuffer = std::move (resampled);
    }

    loadedFileName = file.getFileName();

    // Atomically swap the active buffer. Keep the old one alive until the
    // next load so the audio thread isn't left with a dangling pointer.
    auto* newBuf = stagingBuffer.release();
    previousBuffer.reset (activeBuffer.exchange (newBuf));

    // Reset playback to the start
    readPosition = 0.0;
    normalisedReadPosition.store (0.0f);

    if (playbackMode.load() == PlaybackMode::ContinuousLoop)
        isPlaying.store (true);

    sendChangeMessage();
    return true;
}

// SamplerEngine.cpp
bool SamplerEngine::loadFromMemoryBlock (const void* data, int sizeInBytes,
                                          double targetSampleRate, const juce::String& fileName)
{
    juce::ignoreUnused (targetSampleRate); // buffer was already saved at processing rate

    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

    const int numChannels = stream.readInt();
    const int numSamples  = stream.readInt();

    if (numChannels <= 0 || numSamples <= 0)
        return false;

    auto newBuf = std::make_unique<juce::AudioBuffer<float>> (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
        stream.read (newBuf->getWritePointer (ch),
                     static_cast<int> (numSamples * sizeof (float)));

    previousBuffer.reset (activeBuffer.exchange (newBuf.release()));

    if (! fileName.isEmpty())
        loadedFileName = fileName;

    readPosition = 0.0;
    normalisedReadPosition.store (0.0f);

    if (playbackMode.load() == PlaybackMode::ContinuousLoop)
        isPlaying.store (true);

    sendChangeMessage();
    return true;
}

void SamplerEngine::clearSample()
{
    previousBuffer.reset (activeBuffer.exchange (nullptr));
    isPlaying.store (false);
    readPosition = 0.0;
    normalisedReadPosition.store (0.0f);
    loadedFileName = {};

    sendChangeMessage();
}

void SamplerEngine::saveToMemoryBlock (juce::MemoryBlock& destData) const
{
    const auto* buf = activeBuffer.load();
    if (buf == nullptr)
        return;

    const int totalSamples = buf->getNumSamples();
    const float start = startPoint.load();
    const float end   = endPoint.load();

    const int startSample = juce::jlimit (0, totalSamples - 1,
                                           static_cast<int> (start * (totalSamples - 1)));
    const int endSampleExclusive = juce::jlimit (startSample + 1, totalSamples,
                                                  static_cast<int> (end * (totalSamples - 1)) + 1);
    const int numSamplesToSave = endSampleExclusive - startSample;

    juce::MemoryOutputStream stream (destData, false);

    stream.writeInt (buf->getNumChannels());
    stream.writeInt (numSamplesToSave);

    for (int ch = 0; ch < buf->getNumChannels(); ++ch)
        stream.write (buf->getReadPointer (ch, startSample),
                      static_cast<size_t> (numSamplesToSave) * sizeof (float));
}

bool SamplerEngine::loadFromMemoryBlock (const void* data, int sizeInBytes,
                                          double targetSampleRate)
{
    juce::MemoryInputStream stream (data, static_cast<size_t> (sizeInBytes), false);

    const int numChannels = stream.readInt();
    const int numSamples  = stream.readInt();

    if (numChannels <= 0 || numSamples <= 0)
        return false;

    auto newBuf = std::make_unique<juce::AudioBuffer<float>> (numChannels, numSamples);

    for (int ch = 0; ch < numChannels; ++ch)
        stream.read (newBuf->getWritePointer (ch),
                     static_cast<int> (numSamples * sizeof (float)));

    previousBuffer.reset (activeBuffer.exchange (newBuf.release()));

    readPosition = 0.0;
    normalisedReadPosition.store (0.0f);

    if (playbackMode.load() == PlaybackMode::ContinuousLoop)
        isPlaying.store (true);

    sendChangeMessage();

    return true;
}

//==============================================================================
// Audio thread
//==============================================================================

void SamplerEngine::triggerPlayback()
{
    if (playbackMode.load() == PlaybackMode::ContinuousLoop)
        return;

    const auto* buf = activeBuffer.load();
    if (buf == nullptr)
        return;

    const int numSamples = buf->getNumSamples();
    const float start    = startPoint.load();
    const float end      = endPoint.load();
    const int   startSample = static_cast<int> (start * (numSamples - 1));
    const int   endSample   = static_cast<int> (end   * (numSamples - 1));
    const int   range       = juce::jmax (1, endSample - startSample);

    if (playbackMode.load() == PlaybackMode::KeyTriggerFromRandom)
        readPosition = startSample + (static_cast<double> (std::rand()) / RAND_MAX) * range;
    else
        readPosition = startSample;

    isPlaying.store (true);
}

float SamplerEngine::getNextSampleL()
{
    const auto* buf = activeBuffer.load();
    if (buf == nullptr || !isPlaying.load())
        return 0.0f;

    const int numSamples = buf->getNumSamples();
    const float start    = startPoint.load();
    const float end      = endPoint.load();
    const int   startSample = static_cast<int> (start * (numSamples - 1));
    const int   endSample   = static_cast<int> (end   * (numSamples - 1));

    double pos = reverse.load()
                 ? (endSample - (readPosition - startSample))
                 : readPosition;

    pos = juce::jlimit (0.0, static_cast<double> (numSamples - 1), pos);

    return interpolate (buf->getReadPointer (0), numSamples, pos) * gain.load();
}

float SamplerEngine::getNextSampleR()
{
    const auto* buf = activeBuffer.load();
    if (buf == nullptr || !isPlaying.load())
        return 0.0f;

    const int numSamples    = buf->getNumSamples();
    const float start       = startPoint.load();
    const float end         = endPoint.load();
    const int   startSample = static_cast<int> (start * (numSamples - 1));
    const int   endSample   = static_cast<int> (end   * (numSamples - 1));

    double pos = reverse.load()
                 ? (endSample - (readPosition - startSample))
                 : readPosition;

    pos = juce::jlimit (0.0, static_cast<double> (numSamples - 1), pos);

    // Fall back to left channel for mono samples
    const int channelToRead = juce::jmin (1, buf->getNumChannels() - 1);
    return interpolate (buf->getReadPointer (channelToRead), numSamples, pos) * gain.load();
}

void SamplerEngine::advance (double sampleRate)
{
    juce::ignoreUnused (sampleRate);

    const auto* buf = activeBuffer.load();
    if (buf == nullptr || !isPlaying.load())
        return;

    const int   numSamples  = buf->getNumSamples();
    const float start       = startPoint.load();
    const float end         = endPoint.load();
    const int   startSample = static_cast<int> (start * (numSamples - 1));
    const int   endSample   = static_cast<int> (end   * (numSamples - 1));

    // Pitch ratio from semitones
    pitchRatio = std::pow (2.0, static_cast<double> (pitchSemitones.load()) / 12.0);
    readPosition += pitchRatio;

    // Handle end of region
    const bool pastEnd = readPosition >= endSample;
    if (pastEnd)
    {
        // if (playbackMode.load() == PlaybackMode::ContinuousLoop)
        //     readPosition = startSample; // loop
        // else
        //     isPlaying.store (false);   // one-shot finished
        readPosition = startSample;
    }

    // Update normalised position for UI
    const float normPos = static_cast<float> (readPosition) / juce::jmax (1, numSamples - 1);
    normalisedReadPosition.store (juce::jlimit (0.0f, 1.0f, normPos));
}

//==============================================================================
float SamplerEngine::interpolate (const float* data, int numSamples, double position) const
{
    const int   i0  = static_cast<int> (position);
    const int   i1  = juce::jmin (i0 + 1, numSamples - 1);
    const float frac = static_cast<float> (position - i0);

    return data[i0] + frac * (data[i1] - data[i0]);
}
