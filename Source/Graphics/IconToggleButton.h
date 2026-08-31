// Source/GUI/IconToggleButton.h
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ============================================================
// IconToggleButton
//
// A small square button that draws a hand-built vector glyph
// instead of text, and swaps which glyph it shows based on its
// toggle state. Designed to bind directly to an
// AudioParameterBool via APVTS::ButtonAttachment, same as any
// ordinary juce::ToggleButton/TextButton would.
//
// Each IconGlyph pair represents one axis of playback behaviour
// (e.g. Loop vs. One-shot). The button always shows the icon for
// whichever state is CURRENTLY ACTIVE, with the active/inactive
// pair drawn via drawGlyphFor().
// ============================================================
class IconToggleButton : public juce::Button,
                          private juce::Value::Listener
{
public:
    enum class Glyph
    {
        LoopVsOneShot,      // toggleState true = Loop, false = One-shot
        KeyTriggerVsFreeRun,// toggleState true = Key Trigger, false = Free Run
        StartVsRandom       // toggleState true = Start at Start, false = Start Random
    };

    explicit IconToggleButton (Glyph glyphIn, juce::String tooltipOn, juce::String tooltipOff)
        : juce::Button ({}), glyph (glyphIn), onTip (std::move (tooltipOn)), offTip (std::move (tooltipOff))
    {
        setClickingTogglesState (true);

        // The toggle state can change two ways: a direct click (caught by
        // clicked() below) or a host/preset-driven change coming through the
        // APVTS ButtonAttachment, which sets the state via getToggleStateValue()
        // without going through clicked(). Listening to the Value itself
        // covers both, so the tooltip (and optional live caption) never goes
        // stale regardless of what drove the change.
        getToggleStateValue().addListener (this);
        refreshTooltip();
    }

    ~IconToggleButton() override
    {
        getToggleStateValue().removeListener (this);
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    // Short label describing whichever state is currently active, e.g.
    // "Loop" or "One-shot" — handy for an on-screen caption underneath the
    // icon, in addition to the hover tooltip.
    juce::String getCurrentStateLabel() const
    {
        return getToggleState() ? onTip : offTip;
    }

    // Fired whenever the active state (and therefore the tooltip/label)
    // changes, from a click or from an external attachment. Lets a parent
    // component keep a caption label in sync.
    std::function<void()> onStateLabelChanged;

private:
    void refreshTooltip()
    {
        setTooltip (getCurrentStateLabel());

        if (onStateLabelChanged != nullptr)
            onStateLabelChanged();
    }

    void clicked() override
    {
        juce::Button::clicked();
        refreshTooltip();
    }

    void valueChanged (juce::Value&) override
    {
        refreshTooltip();
    }

    // Individual glyph painters, drawn into a square area, colour supplied by caller
    static void drawLoopGlyph     (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c);
    static void drawOneShotGlyph  (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c);
    static void drawKeyGlyph      (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c);
    static void drawFreeRunGlyph  (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c);
    static void drawStartGlyph    (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c);
    static void drawRandomGlyph   (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c);

    Glyph glyph;
    juce::String onTip, offTip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IconToggleButton)
};