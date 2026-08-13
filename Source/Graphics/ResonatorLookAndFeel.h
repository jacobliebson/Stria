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
    
    // void drawButtonBackground (juce::Graphics& g, juce::Button& button,
    //                         const juce::Colour& /*backgroundColour*/,
    //                         bool shouldDrawButtonAsHighlighted,
    //                         bool shouldDrawButtonAsDown) override;

    // void drawButtonText (juce::Graphics& g,
    //                         juce::TextButton& button,
    //                         bool /*shouldDrawButtonAsHighlighted*/,
    //                         bool /*shouldDrawButtonAsDown*/) override;

    // void drawComboBox (juce::Graphics& g,
    //                         int width, int height,
    //                         bool /*isButtonDown*/,
    //                         int buttonX, int buttonY,
    //                         int buttonW, int buttonH,
    //                         juce::ComboBox& box) override;

    // Scrollbars etc. — kept minimal
    int getSliderThumbRadius (juce::Slider&) override { return 0; }
};
