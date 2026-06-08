// Source/ArpDisplay.cpp
#include "ArpDisplay.h"

ArpDisplay::ArpDisplay (juce::AudioProcessorValueTreeState& apvtsRef,
                         const juce::String& rId,
                         const juce::String& gId,
                         const juce::String& mId,
                         const juce::String& sId,
                         const juce::String& oId)
    : apvts (apvtsRef),
      rateId (rId), gateId (gId), modeId (mId), scatterId (sId), octaveRangeId (oId)
{
    apvts.addParameterListener (gateId,        this);
    apvts.addParameterListener (modeId,        this);
    apvts.addParameterListener (scatterId,     this);
    apvts.addParameterListener (octaveRangeId, this);

    if (auto* p = apvts.getRawParameterValue (gateId))        gate        = p->load();
    if (auto* p = apvts.getRawParameterValue (modeId))        mode        = static_cast<int> (p->load());
    if (auto* p = apvts.getRawParameterValue (scatterId))     scatter     = p->load();
    if (auto* p = apvts.getRawParameterValue (octaveRangeId)) octaveRange = static_cast<int> (p->load());
}

ArpDisplay::~ArpDisplay()
{
    apvts.removeParameterListener (gateId,        this);
    apvts.removeParameterListener (modeId,        this);
    apvts.removeParameterListener (scatterId,     this);
    apvts.removeParameterListener (octaveRangeId, this);
}

void ArpDisplay::parameterChanged (const juce::String& paramId, float newValue)
{
    if      (paramId == gateId)        gate        = newValue;
    else if (paramId == modeId)        mode        = static_cast<int> (newValue);
    else if (paramId == scatterId)     scatter     = newValue;
    else if (paramId == octaveRangeId) octaveRange = static_cast<int> (newValue);

    repaint();
}

std::vector<float> ArpDisplay::buildNotePattern() const
{
    // Build a representative note height pattern for each mode.
    // Heights are normalised to [0,1] where 0 = lowest, 1 = highest.
    // With octaveRange, the pattern spans more vertical space.

    // Base pattern within one octave using 4 representative notes
    // (representative of a typical chord: root, third, fifth, octave)
    const std::vector<float> basePattern = { 0.0f, 0.33f, 0.6f, 1.0f };
    const int                baseSize    = static_cast<int> (basePattern.size());

    std::vector<float> pattern;
    pattern.reserve (numDisplaySteps);

    switch (mode)
    {
        case 0: // Up — ascending through octaves if range > 0
        {
            for (int i = 0; i < numDisplaySteps; ++i)
            {
                int   noteIdx   = i % baseSize;
                float height    = (basePattern[noteIdx]);
                pattern.push_back (juce::jlimit (0.0f, 1.0f, height));
            }
            break;
        }

        case 1: // Down — descending
        {
            for (int i = 0; i < numDisplaySteps; ++i)
            {
                int   noteIdx = i % baseSize;
                float height  = 1.0f - (basePattern[noteIdx]);
                pattern.push_back (juce::jlimit (0.0f, 1.0f, height));
            }
            break;
        }

        case 2: // UpDown — ascend then descend
        {
            const std::vector<float> up   = { 0.0f, 0.33f, 0.6f, 1.0f };
            const std::vector<float> down = { 0.6f, 0.33f, 0.0f, 0.6f };
            std::vector<float> combined;
            combined.insert (combined.end(), up.begin(),   up.end());
            combined.insert (combined.end(), down.begin(), down.end());

            for (int i = 0; i < numDisplaySteps; ++i)
                pattern.push_back (combined[i % static_cast<int> (combined.size())]);
            break;
        }

        default:
            for (int i = 0; i < numDisplaySteps; ++i)
                pattern.push_back (0.5f);
            break;
    }

    return pattern;
}

void ArpDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (6.0f);

    // Background
    g.setColour (ResonatorPalette::backgroundWidget());
    g.fillRoundedRectangle (bounds, 4.0f);

    auto inner       = bounds.reduced (6.0f);
    auto pattern     = buildNotePattern();
    const int   n    = static_cast<int> (pattern.size());
    const float colW = inner.getWidth() / static_cast<float> (n);
    const float barW = colW * gate * 0.85f; // gate length controls bar width

    for (int i = 0; i < n; ++i)
    {
        const float height = pattern[i] * inner.getHeight() * 0.8f + inner.getHeight() * 0.1f;

        // Scatter shifts the bar horizontally within its column
        const float maxShift    = colW;
        const float scatterShift = scatter * (((i * 7 + 3) % 5) - 2.0f) / 5.0f * maxShift;

        const float xPos  = inner.getX() + static_cast<float> (i) * colW + scatterShift;
        const float yPos  = inner.getBottom() - height;
        const float barH  = juce::jmax (3.0f, inner.getHeight() * 0.08f);

        juce::Rectangle<float> bar (xPos + (colW - barW) * 0.5f, yPos, barW, barH);

        g.setColour (ResonatorPalette::accentPrimary());
        g.fillRoundedRectangle (bar, 2.0f);

        g.setColour (ResonatorPalette::accentPrimary().withAlpha (0.25f));
        g.drawLine (bar.getCentreX(), bar.getBottom(),
                    bar.getCentreX(), inner.getBottom(), 1.0f);
    }
}
