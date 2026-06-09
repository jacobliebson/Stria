// Source/EnvelopeDisplay.cpp
#include "EnvelopeDisplay.h"

EnvelopeDisplay::EnvelopeDisplay (juce::AudioProcessorValueTreeState& apvtsRef,
                                   const juce::String& aId,
                                   const juce::String& dId,
                                   const juce::String& sId,
                                   const juce::String& rId)
    : apvts (apvtsRef),
      attackId (aId), decayId (dId), sustainId (sId), releaseId (rId)
{
    apvts.addParameterListener (attackId,  this);
    apvts.addParameterListener (decayId,   this);
    apvts.addParameterListener (sustainId, this);
    apvts.addParameterListener (releaseId, this);

    // Initialise from current parameter values
    if (auto* p = apvts.getRawParameterValue (attackId))  attack  = p->load();
    if (auto* p = apvts.getRawParameterValue (decayId))   decay   = p->load();
    if (auto* p = apvts.getRawParameterValue (sustainId)) sustain = p->load() / 100.0f;
    if (auto* p = apvts.getRawParameterValue (releaseId)) release = p->load();
}

EnvelopeDisplay::~EnvelopeDisplay()
{
    apvts.removeParameterListener (attackId,  this);
    apvts.removeParameterListener (decayId,   this);
    apvts.removeParameterListener (sustainId, this);
    apvts.removeParameterListener (releaseId, this);
}

void EnvelopeDisplay::parameterChanged (const juce::String& paramId, float newValue)
{
    if      (paramId == attackId)  attack  = newValue;
    else if (paramId == decayId)   decay   = newValue;
    else if (paramId == sustainId) sustain = newValue / 100.0f;
    else if (paramId == releaseId) release = newValue;

    repaint();
}

juce::Path EnvelopeDisplay::buildCurve (juce::Rectangle<float> bounds) const
{
    // Normalise each segment to [0,1] then map to pixel widths
    // Sustain segment is always a fixed proportion of the display
    const float totalTime   = (attack / maxAttack) + (decay / maxDecay)
                              + 0.2f + (release / maxRelease);

    const float attackNorm  = (attack  / maxAttack)  / totalTime;
    const float decayNorm   = (decay   / maxDecay)   / totalTime;
    const float sustainNorm = 0.2f / totalTime;
    const float releaseNorm = (release / maxRelease) / totalTime;

    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float x = bounds.getX();
    const float y = bounds.getY();

    const float x0 = x;
    const float x1 = x  + attackNorm  * w;
    const float x2 = x1 + decayNorm   * w;
    const float x3 = x2 + sustainNorm * w;
    const float x4 = x3 + releaseNorm * w;

    const float yBottom  = y + h;
    const float yTop     = y;
    const float ySustain = y + h * (1.0f - sustain);

    juce::Path path;
    path.startNewSubPath (x0, yBottom);
    path.lineTo          (x1, yTop);
    path.lineTo          (x2, ySustain);
    path.lineTo          (x3, ySustain);
    path.lineTo          (x4, yBottom);

    return path;
}


void EnvelopeDisplay::setParameters (const juce::String& newAttackId,
                                      const juce::String& newDecayId,
                                      const juce::String& newSustainId,
                                      const juce::String& newReleaseId)
{
    // Remove old listeners
    apvts.removeParameterListener (attackId,  this);
    apvts.removeParameterListener (decayId,   this);
    apvts.removeParameterListener (sustainId, this);
    apvts.removeParameterListener (releaseId, this);

    // Update IDs
    attackId  = newAttackId;
    decayId   = newDecayId;
    sustainId = newSustainId;
    releaseId = newReleaseId;

    // Read current values for new parameters
    if (auto* p = apvts.getRawParameterValue (attackId))  attack  = p->load();
    if (auto* p = apvts.getRawParameterValue (decayId))   decay   = p->load();
    if (auto* p = apvts.getRawParameterValue (sustainId)) sustain = p->load() / 100.0f;
    if (auto* p = apvts.getRawParameterValue (releaseId)) release = p->load();

    // Add new listeners
    apvts.addParameterListener (attackId,  this);
    apvts.addParameterListener (decayId,   this);
    apvts.addParameterListener (sustainId, this);
    apvts.addParameterListener (releaseId, this);

    repaint();
}

void EnvelopeDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (6.0f);

    // Background
    g.setColour (ResonatorPalette::backgroundWidget());
    g.fillRoundedRectangle (bounds, 4.0f);

    // Filled region under the curve
    auto curvePath = buildCurve (bounds.reduced (4.0f));
    auto filledPath = curvePath;
    filledPath.lineTo (bounds.getRight() - 4.0f, bounds.getBottom() - 4.0f);
    filledPath.lineTo (bounds.getX()     + 4.0f, bounds.getBottom() - 4.0f);
    filledPath.closeSubPath();

    g.setColour (ResonatorPalette::accentPrimary().withAlpha (0.15f));
    g.fillPath (filledPath);

    // Curve line
    g.setColour (ResonatorPalette::accentPrimary());
    g.strokePath (curvePath, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
}
