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

    if (buffer == nullptr || buffer->getNumSamples() == 0)
    {
        waveformDirty = true;
        repaint();
        return;
    }

    // Downsample to a fixed number of peaks for drawing
    const int numPeaks   = 512;
    const int numSamples = buffer->getNumSamples();
    const int blockSize  = juce::jmax (1, numSamples / numPeaks);

    waveformPeaks.reserve (numPeaks);

    for (int i = 0; i < numPeaks; ++i)
    {
        const int startSample = i * blockSize;
        const int endSample   = juce::jmin (startSample + blockSize, numSamples);

        float peak = 0.0f;
        for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
        {
            const float* data = buffer->getReadPointer (ch);
            for (int s = startSample; s < endSample; ++s)
                peak = juce::jmax (peak, std::abs (data[s]));
        }

        waveformPeaks.push_back (peak);
    }

    waveformDirty = true;
    repaint();
}

void SamplerWaveformDisplay::rebuildWaveformPath()
{

    trimStartPos = engine.getStartPoint();
    trimEndPos = engine.getEndPoint();

    startHandlePos = 0.0f;
    endHandlePos = 1.0f;

    waveformPath.clear();

    if (waveformPeaks.empty())
        return;

    auto inner = getInnerBounds();
    const float midY  = inner.getCentreY();
    const float halfH = inner.getHeight() * 0.45f;
    const int   n     = static_cast<int> (waveformPeaks.size());

    

    int startIndex = (int)(n * trimStartPos);
    int endIndex = (int)(n * trimEndPos);

    waveformPath.startNewSubPath (inner.getX(), midY);

    // Top half
    for (int i = startIndex; i < endIndex; ++i)
    {
        const float x = inner.getX() + (inner.getWidth() * (i - startIndex) / (endIndex - startIndex));
        waveformPath.lineTo (x, midY - waveformPeaks[i] * halfH);
    }

    // Bottom half (mirror)
    for (int i = endIndex; i >= startIndex; --i)
    {
        const float x = inner.getX() + (inner.getWidth() * (i - startIndex) / (endIndex - startIndex));
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

    if (waveformPeaks.empty())
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
    if (waveformPeaks.empty())
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
    if (activeDrag == DragTarget::None || waveformPeaks.empty())
        return;

    const float normX = pixelToNormalisedX (static_cast<float> (e.x));

    if (activeDrag == DragTarget::StartHandle)
    {
        const float clamped = juce::jlimit (0.0f, endHandlePos - 0.01f, normX);
        engine.setStartPoint (trimmedToFull(clamped));
        startHandlePos = clamped;

        if (onStartPointChanged) onStartPointChanged (trimmedToFull(clamped));
    }
    else if (activeDrag == DragTarget::EndHandle)
    {
        const float clamped = juce::jlimit (startHandlePos + 0.01f, 1.0f, normX);
        engine.setEndPoint (trimmedToFull(clamped));
        endHandlePos = clamped;
        if (onEndPointChanged) onEndPointChanged (trimmedToFull(clamped));
    }

    repaint();

}

void SamplerWaveformDisplay::mouseUp (const juce::MouseEvent&)
{
    activeDrag = DragTarget::None;
    //waveformDirty = true;
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

