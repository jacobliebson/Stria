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
class IconToggleButton : public juce::Button
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
        setTooltip (onTip);
    }

    void paintButton (juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    void refreshTooltip()
    {
        setTooltip (getToggleState() ? onTip : offTip);
    }

    void clicked() override
    {
        juce::Button::clicked();
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
