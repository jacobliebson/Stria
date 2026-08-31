// Source/ResonatorLookAndFeel.h
#pragma once

#include "ResonatorPalette.h"

#include <juce_audio_processors/juce_audio_processors.h>






class ResonatorLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ResonatorLookAndFeel();

    //==============================================================================
    // Rotary sliders (standard and discrete knobs share this path)
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
    
    // Linear sliders
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                       float sliderPos, float minSliderPos, float maxSliderPos,
                       const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Labels
    void drawLabel (juce::Graphics&, juce::Label&) override;

    // Toggle buttons
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    // Text buttons.
    //
    // NB: this ONLY changes the background/border chrome and font — it still
    // reads the colour supplied by JUCE (which itself comes from whatever
    // buttonColourId/buttonOnColourId the button currently has set). Buttons
    // like Legato/Retrigger and Chord/Arp pick their "selected" look by
    // swapping buttonColourId in application code (see
    // PluginEditor::switchTriggerModeTo/switchEnvelopeTo), not by relying on
    // this LookAndFeel, so that behaviour is untouched — this just makes
    // whatever colour they hand us render as a calmer, more consistent pill
    // instead of the stock JUCE gradient button.
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g,
                            juce::TextButton& button,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

    // Combo box
    void drawComboBox (juce::Graphics& g,
                            int width, int height,
                            bool isButtonDown,
                            int buttonX, int buttonY,
                            int buttonW, int buttonH,
                            juce::ComboBox& box) override;

    // Keeps the combo box's text label inset from the left edge instead of
    // sitting flush against it.
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override;

    // Scrollbars etc. — kept minimal
    int getSliderThumbRadius (juce::Slider&) override { return 0; }
};