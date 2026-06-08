// Source/ResonatorLookAndFeel.cpp
#include "ResonatorLookAndFeel.h"

ResonatorLookAndFeel::ResonatorLookAndFeel()
{
    // Background colours
    setColour (juce::ResizableWindow::backgroundColourId, ResonatorPalette::backgroundDeep());
    setColour (juce::DocumentWindow::backgroundColourId,  ResonatorPalette::backgroundDeep());

    // Label colours
    setColour (juce::Label::textColourId, ResonatorPalette::textPrimary());

    // Slider colours (used as fallback; drawRotarySlider overrides visuals)
    setColour (juce::Slider::rotarySliderFillColourId,    ResonatorPalette::accentPrimary());
    setColour (juce::Slider::rotarySliderOutlineColourId, ResonatorPalette::knobOutline());
    setColour (juce::Slider::thumbColourId,               ResonatorPalette::knobIndicator());
    setColour (juce::Slider::textBoxTextColourId,         ResonatorPalette::textSecondary());
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,    ResonatorPalette::accentPrimary().withAlpha (0.3f));

    // Toggle button
    setColour (juce::ToggleButton::textColourId,          ResonatorPalette::textPrimary());
    setColour (juce::ToggleButton::tickColourId,          ResonatorPalette::accentPrimary());
    setColour (juce::ToggleButton::tickDisabledColourId,  ResonatorPalette::textSecondary());
}

void ResonatorLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                              int x, int y, int width, int height,
                                              float sliderPosProportional,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider& slider)
{
    const float radius    = static_cast<float> (juce::jmin (width, height)) * 0.5f - 4.0f;
    const float centreX   = static_cast<float> (x) + static_cast<float> (width)  * 0.5f;
    const float centreY   = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
    const float angle     = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const bool  isDiscrete = (slider.getInterval() >= 1.0 && slider.getMaximum() - slider.getMinimum() <= 10.0);

    // Arc track
    {
        juce::Path track;
        track.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (ResonatorPalette::backgroundWidget());
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Filled arc (value)
    {
        juce::Path fill;
        fill.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                            rotaryStartAngle, angle, true);
        g.setColour (ResonatorPalette::accentPrimary());
        g.strokePath (fill, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Knob body
    {
        juce::Path knob;
        const float knobRadius = radius * 0.65f;
        knob.addEllipse (centreX - knobRadius, centreY - knobRadius,
                         knobRadius * 2.0f, knobRadius * 2.0f);

        g.setColour (ResonatorPalette::knobBody());
        g.fillPath (knob);

        g.setColour (ResonatorPalette::knobOutline());
        g.strokePath (knob, juce::PathStrokeType (1.5f));
    }

    // Indicator line
    {
        const float indicatorLength = radius * 0.55f;
        const float innerRadius     = radius * 0.18f;
        const float ix = centreX + innerRadius * std::sin (angle);
        const float iy = centreY - innerRadius * std::cos (angle);
        const float ox = centreX + indicatorLength * std::sin (angle);
        const float oy = centreY - indicatorLength * std::cos (angle);

        g.setColour (isDiscrete ? ResonatorPalette::accentPrimary()
                                : ResonatorPalette::knobIndicator());
        g.drawLine (ix, iy, ox, oy, 2.0f);
    }

    // Discrete mode: draw tick marks at each step
    if (isDiscrete)
    {
        const int   numSteps  = static_cast<int> (slider.getMaximum() - slider.getMinimum()) + 1;
        const float tickInner = radius * 0.82f;
        const float tickOuter = radius * 0.98f;

        for (int i = 0; i < numSteps; ++i)
        {
            const float t        = static_cast<float> (i) / static_cast<float> (numSteps - 1);
            const float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            const float tx1 = centreX + tickInner * std::sin (tickAngle);
            const float ty1 = centreY - tickInner * std::cos (tickAngle);
            const float tx2 = centreX + tickOuter * std::sin (tickAngle);
            const float ty2 = centreY - tickOuter * std::cos (tickAngle);

            const bool isCurrent = (std::abs (t - sliderPosProportional) < (0.5f / static_cast<float> (numSteps - 1)));
            g.setColour (isCurrent ? ResonatorPalette::accentPrimary()
                                   : ResonatorPalette::knobOutline());
            g.drawLine (tx1, ty1, tx2, ty2, 1.5f);
        }
    }
}

void ResonatorLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (ResonatorPalette::textSecondary());
    g.setFont (juce::Font (juce::FontOptions().withHeight (12.0f)));
    g.drawFittedText (label.getText(),
                      label.getLocalBounds(),
                      label.getJustificationType(),
                      1, 1.0f);
}

void ResonatorLookAndFeel::drawToggleButton (juce::Graphics& g,
                                              juce::ToggleButton& button,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool /*shouldDrawButtonAsDown*/)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted);

    const auto bounds  = button.getLocalBounds().toFloat().reduced (2.0f);
    const float corner = bounds.getHeight() * 0.5f;
    const bool  isOn   = button.getToggleState();

    // Pill background
    g.setColour (isOn ? ResonatorPalette::accentPrimary()
                      : ResonatorPalette::backgroundWidget());
    g.fillRoundedRectangle (bounds, corner);

    // Outline
    g.setColour (isOn ? ResonatorPalette::accentPrimary()
                      : ResonatorPalette::knobOutline());
    g.drawRoundedRectangle (bounds, corner, 1.5f);

    // Label
    g.setColour (ResonatorPalette::textPrimary());
    g.setFont (juce::Font (juce::FontOptions().withHeight (12.0f)));
    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds(),
                      juce::Justification::centred, 1);
}
