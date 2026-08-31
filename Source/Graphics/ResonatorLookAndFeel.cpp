// Source/ResonatorLookAndFeel.cpp
#include "ResonatorLookAndFeel.h"
#include "ResonatorPalette.h"


ResonatorLookAndFeel::ResonatorLookAndFeel()
{
    // Background colours
    setColour (juce::ResizableWindow::backgroundColourId, ResonatorPalette::backgroundDeep());
    setColour (juce::DocumentWindow::backgroundColourId,  ResonatorPalette::backgroundDeep());

    // Label colours
    setColour (juce::Label::textColourId, ResonatorPalette::textSecondary());

    // Slider colours (used as fallback; drawRotarySlider overrides visuals)
    setColour (juce::Slider::rotarySliderFillColourId,    ResonatorPalette::accentPrimary());
    setColour (juce::Slider::rotarySliderOutlineColourId, ResonatorPalette::knobOutline());
    setColour (juce::Slider::thumbColourId,               ResonatorPalette::knobIndicator());
    setColour (juce::Slider::textBoxTextColourId,         ResonatorPalette::textSecondary());
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,    ResonatorPalette::accentPrimary().withAlpha (0.3f));

    // Toggle button
    setColour (juce::ToggleButton::textColourId,          ResonatorPalette::textSecondary());
    setColour (juce::ToggleButton::tickColourId,          ResonatorPalette::accentPrimary());
    setColour (juce::ToggleButton::tickDisabledColourId,  ResonatorPalette::textSecondary());

    // Text button defaults — individual buttons (tabs, etc.) can still
    // override buttonColourId/buttonOnColourId per-instance as before.
    setColour (juce::TextButton::buttonColourId,   ResonatorPalette::backgroundWidget());
    setColour (juce::TextButton::buttonOnColourId, ResonatorPalette::accentPrimary().withAlpha (0.3f));
    setColour (juce::TextButton::textColourOnId,   ResonatorPalette::textSecondary());
    setColour (juce::TextButton::textColourOffId,  ResonatorPalette::textSecondary());

    // Combo box
    setColour (juce::ComboBox::backgroundColourId, ResonatorPalette::backgroundWidget());
    setColour (juce::ComboBox::outlineColourId,    ResonatorPalette::knobOutline());
    setColour (juce::ComboBox::textColourId,       ResonatorPalette::textSecondary());
    setColour (juce::ComboBox::arrowColourId,      ResonatorPalette::textSecondary());
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

    // Filled arc (value) — colour is per-slider via rotarySliderFillColourId
    {
        juce::Path fill;
        fill.addCentredArc (centreX, centreY, radius, radius, 0.0f,
                            rotaryStartAngle, angle, true);

        juce::ColourGradient gradient(
            ResonatorPalette::accentPrimary(),    // Start colour (low value/bottom)
            (float)x, (float)y + (float)height, 
            ResonatorPalette::accentSecondary(),    // End colour (high value/top)
            (float)x + (float)width, (float)y, 
            false // isRadial = false, makes it a linear gradient
        );

        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        //g.setGradientFill(gradient);
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

        g.setColour (ResonatorPalette::knobIndicator());
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
            g.setColour (isCurrent ? slider.findColour (juce::Slider::rotarySliderFillColourId)
                                   : ResonatorPalette::knobOutline());
            g.drawLine (tx1, ty1, tx2, ty2, 1.5f);
        }
    }
}

void ResonatorLookAndFeel::drawLinearSlider (juce::Graphics& g, 
                                             int x, int y, int width, int height,
                                             float sliderPos, 
                                             float minSliderPos, 
                                             float maxSliderPos,
                                             const juce::Slider::SliderStyle style, 
                                             juce::Slider& slider)
{
    if (slider.isBar())
        return;

    const bool isVertical = slider.isVertical();
    const float trackWidth = 3.0f;
    
    // 1. Shave 6 pixels off the top and bottom bounds right away
    const float inset = 6.0f;
    
    const float startX  = static_cast<float> (x);
    const float startY  = static_cast<float> (y) + inset;
    const float boundsW = static_cast<float> (width);
    const float boundsH = static_cast<float> (height) - (inset * 2.0f);

    // Track background & active fill
    {
        juce::Path backgroundTrack;
        juce::Path fillTrack;

        float x1, x2, y1, y2 = 0.0f;

        if (isVertical)
        {
            const float trackX = startX + boundsW * 0.5f;

            x1 = trackX; y1 = startY;
            x2 = trackX; y2 = startY + boundsH;
            
            backgroundTrack.startNewSubPath (trackX, startY);
            backgroundTrack.lineTo (trackX, startY + boundsH);

            // Clamp the drawing position to our new inset boundaries
            float clampedPos = juce::jlimit (startY, startY + boundsH, sliderPos);
            fillTrack.startNewSubPath (trackX, clampedPos);
            fillTrack.lineTo (trackX, startY + boundsH);
        }
        else
        {
            // Horizontal layout support just in case
            const float insetX = static_cast<float> (x) + inset;
            const float boundsWHz = static_cast<float> (width) - (inset * 2.0f);
            const float trackY = static_cast<float> (y) + static_cast<float> (height) * 0.5f;

            x1 = insetX; y1 = trackY;
            x2 = insetX + boundsWHz; y2 = trackY;

            backgroundTrack.startNewSubPath (insetX, trackY);
            backgroundTrack.lineTo (insetX + boundsWHz, trackY);

            float clampedPos = juce::jlimit (insetX, insetX + boundsWHz, sliderPos);
            fillTrack.startNewSubPath (insetX, trackY);
            fillTrack.lineTo (clampedPos, trackY);
        }

        juce::ColourGradient backgroundGradient (
            ResonatorPalette::accentPrimary().withAlpha(0.3f), x2, y2,  // start colour and position
            ResonatorPalette::accentSecondary().withAlpha(0.3f), x1, y1,  // end colour and position
            false                                        // false = linear, true = radial
        );

        juce::ColourGradient fillGradient (
            ResonatorPalette::accentPrimary(), x2, y2,  // start colour and position
            ResonatorPalette::accentSecondary(), x1, y1,  // end colour and position
            false                                        // false = linear, true = radial
        );
        
        
        // Draw tracks
        g.setGradientFill (backgroundGradient);
        g.strokePath (backgroundTrack, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setGradientFill (fillGradient);
        g.strokePath (fillTrack, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Slider Handle/Thumb
    {
        juce::Path handle;

        if (isVertical)
        {
            const float handleW = 22.0f;
            const float handleH = 8.0f;
            const float handleX = startX + (boundsW - handleW) * 0.5f;
            const float handleY = juce::jlimit (startY, startY + boundsH, sliderPos) - handleH * 0.5f;

            handle.addRoundedRectangle (handleX, handleY, handleW, handleH, 2.0f);
        }
        else
        {
            const float insetX = static_cast<float> (x) + inset;
            const float boundsWHz = static_cast<float> (width) - (inset * 2.0f);
            const float handleW = 8.0f;
            const float handleH = 22.0f;
            const float handleX = juce::jlimit (insetX, insetX + boundsWHz, sliderPos) - handleW * 0.5f;
            const float handleY = static_cast<float> (y) + (static_cast<float> (height) - handleH) * 0.5f;

            handle.addRoundedRectangle (handleX, handleY, handleW, handleH, 2.0f);
        }

        g.setColour (ResonatorPalette::knobBody());
        g.fillPath (handle);

        g.setColour (ResonatorPalette::knobOutline());
        g.strokePath (handle, juce::PathStrokeType (1.5f));
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
                                              bool shouldDrawButtonAsDown)
{
    // Deliberately shares its metrics and flat-fill treatment with
    // drawButtonBackground() below (same inset, same corner radius, no
    // gradient) so a ToggleButton like Reverse reads as the same family of
    // control as the plain TextButtons next to it (Trim/Reset), differing
    // only in that its "on" state fills with the accent colour — the same
    // language the icon toggles use to show they're active.
    const auto bounds  = button.getLocalBounds().toFloat().reduced (0.75f);
    const float corner = juce::jmin (6.0f, bounds.getHeight() * 0.32f);
    const bool  isOn   = button.getToggleState();

    juce::Colour bg = isOn ? ResonatorPalette::accentPrimary().withAlpha (0.35f)
                           : ResonatorPalette::backgroundWidget();
    if (shouldDrawButtonAsDown)
        bg = bg.darker (0.25f);
    else if (shouldDrawButtonAsHighlighted)
        bg = bg.brighter (0.12f);

    g.setColour (bg);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (isOn ? ResonatorPalette::accentPrimary().darker (0.3f)
                      : ResonatorPalette::borderPanel());
    g.drawRoundedRectangle (bounds, corner, 1.0f);

    // Label
    g.setColour (ResonatorPalette::textSecondary());
    g.setFont (juce::Font (juce::FontOptions().withHeight (12.0f)));
    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds(),
                      juce::Justification::centred, 1);
}

//==============================================================================
void ResonatorLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.75f);
    const float corner = juce::jmin (6.0f, bounds.getHeight() * 0.32f);

    // backgroundColour is whatever JUCE resolved from the button's own
    // buttonColourId/buttonOnColourId — so "selected tab" colours set in
    // application code (Legato/Retrigger, Chord/Arp) pass straight through.
    juce::Colour bg = backgroundColour;
    if (shouldDrawButtonAsDown)
        bg = bg.darker (0.25f);
    else if (shouldDrawButtonAsHighlighted)
        bg = bg.brighter (0.12f);

    g.setColour (bg);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (ResonatorPalette::borderPanel());
    g.drawRoundedRectangle (bounds, corner, 1.0f);
}

void ResonatorLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                            bool /*shouldDrawButtonAsHighlighted*/,
                                            bool /*shouldDrawButtonAsDown*/)
{
    const bool useOnColour = button.getToggleState();
    g.setColour (button.findColour (useOnColour ? juce::TextButton::textColourOnId
                                                 : juce::TextButton::textColourOffId)
                       .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
    g.setFont (juce::Font (juce::FontOptions().withHeight (12.0f)));
    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds(),
                      juce::Justification::centred, 1);
}

//==============================================================================
void ResonatorLookAndFeel::drawComboBox (juce::Graphics& g,
                                          int width, int height,
                                          bool /*isButtonDown*/,
                                          int /*buttonX*/, int /*buttonY*/,
                                          int /*buttonW*/, int /*buttonH*/,
                                          juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int> (0, 0, width, height).toFloat().reduced (0.75f);
    const float corner = juce::jmin (6.0f, bounds.getHeight() * 0.32f);

    g.setColour (ResonatorPalette::backgroundWidget());
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (box.isPopupActive() ? ResonatorPalette::accentPrimary().darker (0.2f)
                                     : ResonatorPalette::knobOutline());
    g.drawRoundedRectangle (bounds, corner, 1.2f);

    // Chevron
    const float arrowBoxW = bounds.getHeight() * 0.9f;
    juce::Rectangle<float> arrowZone (bounds.getRight() - arrowBoxW, bounds.getY(), arrowBoxW, bounds.getHeight());
    juce::Path arrow;
    const float aw = arrowZone.getWidth() * 0.32f;
    const float ah = aw * 0.55f;
    const auto  ac = arrowZone.getCentre();
    arrow.startNewSubPath (ac.x - aw, ac.y - ah * 0.5f);
    arrow.lineTo (ac.x,       ac.y + ah * 0.5f);
    arrow.lineTo (ac.x + aw,  ac.y - ah * 0.5f);
    g.setColour (ResonatorPalette::textSecondary());
    g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ResonatorLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    // Default JUCE positioning butts the text right up against the left
    // edge of the box; give it a little breathing room, and keep it clear
    // of the chevron drawn in drawComboBox() on the right.
    const int leftPad  = 8;
    const int rightPad = juce::roundToInt (box.getHeight() * 0.9f) + 4;

    label.setBounds (leftPad, 1,
                     juce::jmax (0, box.getWidth() - leftPad - rightPad),
                     box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}