#include "IconToggleButton.h"
#include "ResonatorPalette.h"

//==============================================================================
void IconToggleButton::paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat();
    const bool active = getToggleState();

    // Background: filled rounded square when active, outlined when inactive.
    const float corner = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.22f;

    juce::Colour bg = active ? ResonatorPalette::accentPrimary()
                              : ResonatorPalette::backgroundPanel();

    if (shouldDrawButtonAsDown)
        bg = bg.darker (0.2f);
    else if (shouldDrawButtonAsHighlighted)
        bg = active ? bg.brighter (0.1f) : bg.brighter (0.35f);

    g.setColour (bg);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (active ? ResonatorPalette::accentPrimary().darker (0.6f)
                         : ResonatorPalette::textSecondary().withAlpha (0.5f));
    g.drawRoundedRectangle (bounds.reduced (0.75f), corner, 1.2f);

    // Glyph colour: dark ink on the lit (accent-filled) state, light ink otherwise.
    const juce::Colour ink = active ? ResonatorPalette::backgroundPanel()
                                     : ResonatorPalette::textSecondary();

    auto glyphArea = bounds.reduced (bounds.getWidth() * 0.24f, bounds.getHeight() * 0.24f);

    switch (glyph)
    {
        case Glyph::LoopVsOneShot:
            active ? drawLoopGlyph (g, glyphArea, ink) : drawOneShotGlyph (g, glyphArea, ink);
            break;
        case Glyph::KeyTriggerVsFreeRun:
            // toggleState is bound directly to SAMPLE_FREE_RUN, where
            // true == Free Run — so the lit/active glyph must be Free Run,
            // not Key Trigger, or the icon shows the opposite of reality.
            active ? drawFreeRunGlyph (g, glyphArea, ink) : drawKeyGlyph (g, glyphArea, ink);
            break;
        case Glyph::StartVsRandom:
            // toggleState is bound directly to SAMPLE_START_RANDOM, where
            // true == Random — lit/active glyph must be Random, not Start.
            active ? drawRandomGlyph (g, glyphArea, ink) : drawStartGlyph (g, glyphArea, ink);
            break;
    }
}

//==============================================================================
// Loop: a circular arrow — an open ring with an arrowhead at one end,
// the universal "repeats" symbol.
void IconToggleButton::drawLoopGlyph (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);

    const auto centre = r.getCentre();
    const float radius = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    const float thickness = radius * 0.28f;

    juce::Path ring;
    // Leave a gap at the top-right for the arrowhead to sit in
    ring.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                        juce::MathConstants<float>::pi * 0.15f,
                        juce::MathConstants<float>::pi * 1.85f, true);
    g.strokePath (ring, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Arrowhead at the ring's start angle
    const float angle = juce::MathConstants<float>::pi * 0.15f;
    const juce::Point<float> tip (centre.x + radius * std::sin (angle), centre.y - radius * std::cos (angle));
    const float headSize = radius * 0.55f;

    juce::Path head;
    head.addTriangle (tip.x - headSize * 0.55f, tip.y - headSize * 0.15f,
                      tip.x + headSize * 0.55f, tip.y - headSize * 0.15f,
                      tip.x, tip.y + headSize * 0.55f);
    g.fillPath (head);
}

// One-shot: a straight arrow running into a stop bar — "play once, then stop".
void IconToggleButton::drawOneShotGlyph (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);

    const float y = r.getCentreY();
    const float thickness = r.getHeight() * 0.16f;
    const float stopBarX = r.getRight() - r.getWidth() * 0.12f;

    juce::Path shaft;
    shaft.startNewSubPath (r.getX(), y);
    shaft.lineTo (stopBarX - r.getWidth() * 0.12f, y);
    g.strokePath (shaft, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path head;
    const float headSize = r.getHeight() * 0.28f;
    const float tipX = stopBarX - r.getWidth() * 0.14f;
    head.addTriangle (tipX - headSize, y - headSize, tipX - headSize, y + headSize, tipX + headSize * 0.4f, y);
    g.fillPath (head);

    g.fillRoundedRectangle (stopBarX - thickness * 0.5f, r.getY(), thickness, r.getHeight(), thickness * 0.4f);
}

// Key trigger: a simple piano-key glyph — two white keys with a black key
// between them, evoking "responds to MIDI notes".
void IconToggleButton::drawKeyGlyph (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);

    const float keyW = r.getWidth() / 3.0f;
    juce::Path outline;
    outline.addRoundedRectangle (r, r.getWidth() * 0.08f);
    g.strokePath (outline, juce::PathStrokeType (r.getWidth() * 0.07f));

    // Dividers between the three white keys
    for (int i = 1; i < 3; ++i)
    {
        const float x = r.getX() + keyW * (float) i;
        g.drawLine (x, r.getY(), x, r.getBottom(), r.getWidth() * 0.05f);
    }

    // Black key sitting on top, offset toward the left divider
    juce::Rectangle<float> blackKey (r.getX() + keyW * 0.62f, r.getY(), keyW * 0.76f, r.getHeight() * 0.6f);
    g.fillRect (blackKey);
}

// Free run: a double chevron — "fast-forward" reads as continuous, running
// on its own without waiting for a key, and stays visually distinct from
// the single-arrow One-shot glyph (no shared shape between the two).
void IconToggleButton::drawFreeRunGlyph (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);

    const float w = r.getWidth();
    const float h = r.getHeight();
    const float thickness = h * 0.16f;
    const auto  centre = r.getCentre();

    auto strokeChevron = [&] (float offsetX)
    {
        const float chevW = w * 0.24f;
        const float chevH = h * 0.34f;
        const float x = centre.x + offsetX;

        juce::Path chevron;
        chevron.startNewSubPath (x - chevW * 0.5f, centre.y - chevH);
        chevron.lineTo (x + chevW * 0.5f, centre.y);
        chevron.lineTo (x - chevW * 0.5f, centre.y + chevH);
        g.strokePath (chevron, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };

    strokeChevron (-w * 0.16f);
    strokeChevron ( w * 0.16f);
}

// Start-at-start: a playhead marker sitting flush against the left edge.
void IconToggleButton::drawStartGlyph (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);

    const float barW = r.getWidth() * 0.16f;
    g.fillRoundedRectangle (r.getX(), r.getY(), barW, r.getHeight(), barW * 0.4f);

    juce::Path head;
    const float headSize = r.getHeight() * 0.32f;
    const float startX = r.getX() + barW + r.getWidth() * 0.08f;
    head.addTriangle (startX, r.getCentreY() - headSize,
                      startX, r.getCentreY() + headSize,
                      startX + headSize * 1.4f, r.getCentreY());
    g.fillPath (head);
}

// Random start: crossed shuffle arrows.
void IconToggleButton::drawRandomGlyph (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);
    const float thickness = r.getHeight() * 0.12f;

    auto strokeArrow = [&] (juce::Point<float> from, juce::Point<float> to)
    {
        juce::Path shaft;
        shaft.startNewSubPath (from);
        shaft.lineTo (to);
        g.strokePath (shaft, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto delta = to - from;
        const float len = std::sqrt (delta.x * delta.x + delta.y * delta.y);
        const auto dir = len > 0.0f ? delta / len : juce::Point<float> (1.0f, 0.0f);
        const auto normal = juce::Point<float> (-dir.y, dir.x);
        const float headSize = r.getHeight() * 0.22f;
        juce::Path head;
        head.addTriangle (to,
                          to - dir * headSize + normal * headSize * 0.6f,
                          to - dir * headSize - normal * headSize * 0.6f);
        g.fillPath (head);
    };

    strokeArrow ({ r.getX(), r.getY() }, { r.getRight(), r.getBottom() });
    strokeArrow ({ r.getX(), r.getBottom() }, { r.getRight(), r.getY() });
}