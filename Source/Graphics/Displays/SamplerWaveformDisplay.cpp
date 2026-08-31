// Source/Graphics/Displays/SamplerWaveformDisplay.cpp
#include "SamplerWaveformDisplay.h"

SamplerWaveformDisplay::SamplerWaveformDisplay (SamplerEngine& eng)
    : engine (eng)
{
    startTimerHz (repaintRateHz);
}

SamplerWaveformDisplay::~SamplerWaveformDisplay()
{
    stopTimer();
}

//==============================================================================
void SamplerWaveformDisplay::sampleLoaded (const juce::AudioBuffer<float>* buffer)
{
    trimStartPos = 0.0f;
    trimEndPos = 1.0f;
    startHandlePos = 0.0f;
    endHandlePos = 1.0f;

    waveformPeaks.clear();
    sourceBuffer.setSize (0, 0);

    if (buffer == nullptr || buffer->getNumSamples() == 0)
    {
        waveformDirty = true;
        repaint();
        return;
    }

    // Keep a copy of the full-resolution audio. We deliberately do NOT
    // decimate here — rebuildWaveformPath() re-decimates from this buffer
    // for whatever range is currently trimmed/visible, so zooming into a
    // small region still yields numDisplayPeaks worth of detail instead of
    // reusing a coarse whole-file decimation.
    sourceBuffer = *buffer;

    waveformDirty = true;
    repaint();
}

//==============================================================================
std::vector<float> SamplerWaveformDisplay::computePeaks (int startSample, int endSample, int numPeaksWanted) const
{
    std::vector<float> peaks;

    const int rangeSamples = endSample - startSample;
    if (rangeSamples <= 0 || sourceBuffer.getNumSamples() == 0)
        return peaks;

    // Don't ask for more peaks than there are samples in the range
    const int numPeaks = juce::jmax (1, juce::jmin (numPeaksWanted, rangeSamples));
    peaks.reserve (numPeaks);

    const int numChannels = sourceBuffer.getNumChannels();
    const int totalSamples = sourceBuffer.getNumSamples();

    for (int i = 0; i < numPeaks; ++i)
    {
        // Use 64-bit intermediate math so this stays accurate on long files
        const int blockStart = startSample + (int) ((juce::int64) i * rangeSamples / numPeaks);
        const int blockEnd   = startSample + (int) ((juce::int64) (i + 1) * rangeSamples / numPeaks);
        const int clampedEnd = juce::jmin (juce::jmax (blockStart + 1, blockEnd), totalSamples);
        const int clampedStart = juce::jmin (blockStart, totalSamples);

        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* data = sourceBuffer.getReadPointer (ch);
            for (int s = clampedStart; s < clampedEnd; ++s)
                peak = juce::jmax (peak, std::abs (data[s]));
        }

        peaks.push_back (peak);
    }

    return peaks;
}

void SamplerWaveformDisplay::rebuildWaveformPath()
{
    trimStartPos = engine.getStartPoint();
    trimEndPos = engine.getEndPoint();

    startHandlePos = 0.0f;
    endHandlePos = 1.0f;

    waveformPath.clear();
    waveformPeaks.clear();

    const int totalSamples = sourceBuffer.getNumSamples();
    if (totalSamples == 0)
    {
        waveformDirty = false;
        return;
    }

    const int startSample = juce::jlimit (0, totalSamples, (int) (totalSamples * trimStartPos.load()));
    const int endSample   = juce::jlimit (startSample, totalSamples, (int) (totalSamples * trimEndPos.load()));

    // Re-decimate from the raw buffer for just the currently trimmed range,
    // so zoomed-in views get full peak resolution rather than a handful of
    // points sliced out of a whole-file decimation.
    waveformPeaks = computePeaks (startSample, endSample, numDisplayPeaks);

    if (waveformPeaks.empty())
    {
        waveformDirty = false;
        return;
    }

    auto inner = getInnerBounds();
    const float midY  = inner.getCentreY();
    const float halfH = inner.getHeight() * 0.45f;
    const int   n     = static_cast<int> (waveformPeaks.size());
    const float denom = juce::jmax (1, n - 1); // avoid divide-by-zero when n == 1

    waveformPath.startNewSubPath (inner.getX(), midY);

    // Top half — waveformPeaks now spans exactly the visible/trimmed range,
    // so we map its indices straight across the full width, no more
    // startIndex/endIndex slicing needed.
    for (int i = 0; i < n; ++i)
    {
        const float x = inner.getX() + (inner.getWidth() * i / denom);
        waveformPath.lineTo (x, midY - waveformPeaks[i] * halfH);
    }

    // Bottom half (mirror)
    for (int i = n - 1; i >= 0; --i)
    {
        const float x = inner.getX() + (inner.getWidth() * i / denom);
        waveformPath.lineTo (x, midY + waveformPeaks[i] * halfH);
    }

    waveformPath.closeSubPath();
    waveformDirty = false;
}

//==============================================================================
void SamplerWaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);

    // Background
    g.setColour (ResonatorPalette::backgroundWidget());
    g.fillRoundedRectangle (bounds, 4.0f);

    if (sourceBuffer.getNumSamples() == 0)
    {
        // Empty state — show drop hint
        g.setColour (ResonatorPalette::textSecondary());
        g.setFont (juce::Font (juce::FontOptions().withHeight (13.0f)));
        g.drawText ("Drop an audio file here", bounds, juce::Justification::centred);
        return;
    }

    if (waveformDirty)
        rebuildWaveformPath();

    auto inner = getInnerBounds();

    // Shaded region between start and end handles
    const float startX = normalisedXToPixel (startHandlePos);
    const float endX   = normalisedXToPixel (endHandlePos);

    g.setColour (ResonatorPalette::accentPrimary().withAlpha (0.08f));
    g.fillRect  (juce::Rectangle<float> (inner.getX(), inner.getY(),
                                          startX - inner.getX(), inner.getHeight()));
    g.setColour (ResonatorPalette::accentPrimary().withAlpha (0.08f));
    g.fillRect  (juce::Rectangle<float> (endX, inner.getY(),
                                          inner.getRight() - endX, inner.getHeight()));

    // Waveform — gradient fill
    juce::ColourGradient waveGrad (
        ResonatorPalette::accentSecondary().withAlpha (0.7f), 0.0f, inner.getY(),
        ResonatorPalette::accentPrimary().withAlpha (0.7f),   0.0f, inner.getBottom(),
        false);

    g.setGradientFill (waveGrad);
    g.fillPath (waveformPath);

    // Start handle
    g.setColour (ResonatorPalette::accentPrimary());
    g.fillRect (juce::Rectangle<float> (startX - handleWidth * 0.5f, inner.getY(),
                                         handleWidth, inner.getHeight()));

    // End handle
    g.setColour (ResonatorPalette::accentSecondary());
    g.fillRect (juce::Rectangle<float> (endX - handleWidth * 0.5f, inner.getY(),
                                         handleWidth, inner.getHeight()));

    // Playhead
    if (engine.isPlaying.load())
    {
        const float playX = normalisedXToPixel ((engine.normalisedReadPosition.load() - trimStartPos) / (trimEndPos - trimStartPos));
        g.setColour (ResonatorPalette::textPrimary().withAlpha (0.8f));
        g.drawVerticalLine (static_cast<int> (playX), inner.getY(), inner.getBottom());
    }
}

void SamplerWaveformDisplay::resized()
{
    waveformDirty = true;
}

void SamplerWaveformDisplay::resetTrim()
{
    trimStartPos = 0.0f;
    trimEndPos = 1.0f;
    startHandlePos = 0.0f;
    endHandlePos = 1.0f;
}

//==============================================================================
// File drag and drop
//==============================================================================

bool SamplerWaveformDisplay::isInterestedInFileDrag (const juce::StringArray& files)
{
    juce::AudioFormatManager fmt;
    fmt.registerBasicFormats();

    for (const auto& path : files)
    {
        juce::File f (path);
        if (fmt.findFormatForFileExtension (f.getFileExtension()) != nullptr)
            return true;
    }
    return false;
}

void SamplerWaveformDisplay::filesDropped (const juce::StringArray& files, int, int)
{
    if (files.isEmpty())
        return;

    if (onFileDropped)
        onFileDropped (juce::File (files[0]));
}

//==============================================================================
// Handle dragging
//==============================================================================

void SamplerWaveformDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (sourceBuffer.getNumSamples() == 0)
        return;

    const float startX = normalisedXToPixel (startHandlePos);
    const float endX   = normalisedXToPixel (endHandlePos);
    const float mx     = static_cast<float> (e.x);

    if (std::abs (mx - startX) < handleWidth * 2.0f)
        activeDrag = DragTarget::StartHandle;
    else if (std::abs (mx - endX) < handleWidth * 2.0f)
        activeDrag = DragTarget::EndHandle;
    else
        activeDrag = DragTarget::None;
}

void SamplerWaveformDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (activeDrag == DragTarget::None || sourceBuffer.getNumSamples() == 0)
        return;

    const float normX = pixelToNormalisedX (static_cast<float> (e.x));

    if (activeDrag == DragTarget::StartHandle)
    {
        const float clamped = juce::jlimit (0.0f, endHandlePos - 0.01f, normX);
        startHandlePos = clamped;

        // Don't touch the engine directly here — report the change and let
        // SamplerPanel route it through the APVTS (SAMPLE_TRIM_START), which
        // both applies it to the engine and persists it. Writing to the
        // engine directly, as this used to do, meant drags never reached
        // the parameter and were lost on save/reload.
        if (onStartPointChanged) onStartPointChanged (trimmedToFull (clamped));
    }
    else if (activeDrag == DragTarget::EndHandle)
    {
        const float clamped = juce::jlimit (startHandlePos + 0.01f, 1.0f, normX);
        endHandlePos = clamped;
        if (onEndPointChanged) onEndPointChanged (trimmedToFull (clamped));
    }

    repaint();

}

void SamplerWaveformDisplay::mouseUp (const juce::MouseEvent&)
{
    activeDrag = DragTarget::None;

    repaint();
}

//==============================================================================
// Helpers
//==============================================================================

void SamplerWaveformDisplay::timerCallback()
{
    if (engine.isPlaying.load())
        repaint();
}

juce::Rectangle<float> SamplerWaveformDisplay::getInnerBounds() const
{
    return getLocalBounds().toFloat().reduced (6.0f);
}

float SamplerWaveformDisplay::normalisedXToPixel (float normX) const
{
    auto inner = getInnerBounds();
    return inner.getX() + normX * inner.getWidth();
}

float SamplerWaveformDisplay::pixelToNormalisedX (float pixelX) const
{
    auto inner = getInnerBounds();
    return (pixelX - inner.getX()) / inner.getWidth();
}

float SamplerWaveformDisplay::trimmedToFull (float trimmedX)
{
    return (trimEndPos - trimStartPos) * trimmedX + trimStartPos;
}