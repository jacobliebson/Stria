// Source/PluginEditor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Layout constants
namespace Layout
{
    constexpr int margin        = 12;
    constexpr int windowW   = 920;
    constexpr int windowH   = 650;  // reduced from 620 - 480
    constexpr int topH      = 307;  // reduced from 140
    constexpr int bottomH   = windowH - topH - margin * 3;
    constexpr int panelPadding  = 10;
    constexpr int titleHeight   = 22;
    constexpr int knobSize      = 50;
    constexpr int labelHeight   = 16;
    constexpr int knobStride    = knobSize + labelHeight + 6;
    constexpr int bypassW       = 72;
    constexpr int bypassH       = 24;

}

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&lookAndFeel);

    auto& apvts = audioProcessor.apvts;

    // Resonator
    setupKnob (feedbackKnob, feedbackLabel, "Feedback",
               feedbackAttachment, "FEEDBACK");
    setupKnob (dampingKnob,  dampingLabel,  "Damping",
               dampingAttachment,  "DAMPING");

    // Trigger
    setupKnob (thresholdKnob, thresholdLabel, "Threshold",
               thresholdAttachment, "TRIG_THRESHOLD");
    setupKnob (releaseKnob,   releaseLabel,   "Release",
               releaseAttachment,   "TRIG_RELEASE");
    setupKnob (softnessKnob,  softnessLabel,  "Softness",
               softnessAttachment,  "TRIG_SOFTNESS");

    // Envelope
    envelopeDisplay = std::make_unique<EnvelopeDisplay> (apvts,
                          "ENV_ATTACK", "ENV_DECAY", "ENV_SUSTAIN", "ENV_RELEASE");
    addAndMakeVisible (*envelopeDisplay);

    setupKnob (attackKnob,     attackLabel,     "Attack",
               attackAttachment,     "ENV_ATTACK");
    setupKnob (decayKnob,      decayLabel,      "Decay",
               decayAttachment,      "ENV_DECAY");
    setupKnob (sustainKnob,    sustainLabel,    "Sustain",
               sustainAttachment,    "ENV_SUSTAIN");
    setupKnob (releaseEnvKnob, releaseEnvLabel, "Release",
               releaseEnvAttachment, "ENV_RELEASE");

    // Arpeggiator
    arpDisplay = std::make_unique<ArpDisplay> (apvts,
                     "ARP_RATE", "ARP_GATE", "ARP_MODE",
                     "ARP_SCATTER", "ARP_RANGE");
    addAndMakeVisible (*arpDisplay);

    setupDiscreteKnob (rateKnob,       rateLabel,       "Rate",
                   rateAttachment,       "ARP_RATE");
    setupKnob         (gateKnob,       gateLabel,       "Gate",
                       gateAttachment,       "ARP_GATE");
    setupDiscreteKnob (modeKnob,       modeLabel,       "Mode",
                       modeAttachment,       "ARP_MODE");
    setupKnob         (deviationKnob,  deviationLabel,  "Deviation",
                       deviationAttachment,  "ARP_DEVIATION");
    setupKnob         (scatterKnob,    scatterLabel,    "Scatter",
                       scatterAttachment,    "ARP_SCATTER");
    setupDiscreteKnob (octaveRangeKnob, octaveRangeLabel, "Oct Range",
                       octaveRangeAttachment, "ARP_RANGE");

    // Mixer
    arpGainSlider.setSliderStyle   (juce::Slider::LinearVertical);
    chordGainSlider.setSliderStyle (juce::Slider::LinearVertical);
    arpGainSlider.setTextBoxStyle   (juce::Slider::TextBoxBelow, false, 50, 16);
    chordGainSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 16);
    addAndMakeVisible (arpGainSlider);
    addAndMakeVisible (chordGainSlider);
    arpGainLabel.setText   ("Arp",   juce::dontSendNotification);
    chordGainLabel.setText ("Chord", juce::dontSendNotification);
    arpGainLabel.setJustificationType   (juce::Justification::centred);
    chordGainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (arpGainLabel);
    addAndMakeVisible (chordGainLabel);
    arpGainAttachment   = std::make_unique<SliderAttachment> (apvts, "ARP_GAIN",   arpGainSlider);
    chordGainAttachment = std::make_unique<SliderAttachment> (apvts, "CHORD_GAIN", chordGainSlider);

    setupKnob (mixKnob, mixLabel, "Mix", mixAttachment, "MIX");

    // Bypass
    //bypassButton.setButtonText ("Bypass");
    //addAndMakeVisible (bypassButton);
    //bypassAttachment = std::make_unique<ButtonAttachment> (apvts, "ARP_BYPASS", bypassButton);

    startTimerHz (30);

    setSize (Layout::windowW, Layout::windowH);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::setupKnob (juce::Slider& slider,
                                                   juce::Label& label,
                                                   const juce::String& labelText,
                                                   std::unique_ptr<SliderAttachment>& attachment,
                                                   const juce::String& paramId)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 14);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);

    attachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, paramId, slider);
}

void AudioPluginAudioProcessorEditor::setupDiscreteKnob (juce::Slider& slider,
                                                           juce::Label& label,
                                                           const juce::String& labelText,
                                                           std::unique_ptr<SliderAttachment>& attachment,
                                                           const juce::String& paramId)
{
    setupKnob (slider, label, labelText, attachment, paramId);
    slider.setScrollWheelEnabled (true);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (audioProcessor.apvts.getParameter (paramId)))
    {
        slider.setRange (0.0, static_cast<double> (choice->choices.size() - 1), 1.0);
    }
    else if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (audioProcessor.apvts.getParameter (paramId)))
    {
        auto range = param->getNormalisableRange();
        slider.setRange (static_cast<double> (range.start),
                         static_cast<double> (range.end),
                         static_cast<double> (range.interval));
    }
}

void AudioPluginAudioProcessorEditor::drawPanel (juce::Graphics& g,
                                                   juce::Rectangle<int> bounds,
                                                   const juce::String& title)
{
    auto b = bounds.toFloat();

    // Panel background
    g.setColour (ResonatorPalette::backgroundPanel());
    g.fillRoundedRectangle (b, 8.0f);

    // Panel border
    g.setColour (ResonatorPalette::borderPanel());
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

    // Title
    g.setColour (ResonatorPalette::textSecondary());
    g.setFont (juce::Font (juce::FontOptions().withHeight (11.0f)));
    g.drawText (title.toUpperCase(),
                bounds.getX() + Layout::panelPadding,
                bounds.getY() + 6,
                bounds.getWidth() - Layout::panelPadding * 2,
                12,
                juce::Justification::left);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ResonatorPalette::backgroundDeep());

    const int m  = Layout::margin;
    const int bH = Layout::bottomH;
    const int tH = Layout::topH;
    const int w  = getWidth();

    // Resonator panel (top left)
    const int resonatorW = 200;
    drawPanel (g, { m, m, resonatorW, tH }, "Resonator");

    // Bottom panels — equal width
    const int numPanels  = 4;
    const int panelW     = (w - m * (numPanels + 1)) / numPanels;
    const int panelY     = m + tH + m;

    drawPanel (g, { m,                           panelY, panelW, bH }, "Trigger");
    drawPanel (g, { m * 2 + panelW,              panelY, panelW, bH }, "Envelopes");
    drawPanel (g, { m * 3 + panelW * 2,          panelY, panelW, bH }, "Arpeggiator");
    drawPanel (g, { m * 4 + panelW * 3,          panelY, panelW, bH }, "Mixer");
}

void AudioPluginAudioProcessorEditor::resized()
{
    if (arpDisplay == nullptr || envelopeDisplay == nullptr)
        return;

    const int m      = Layout::margin;
    const int tH     = Layout::topH;
    const int bH     = Layout::bottomH;
    const int w      = getWidth();
    const int kS     = Layout::knobSize;
    const int lH     = Layout::labelHeight;
    const int stride = Layout::knobStride;
    const int pad    = Layout::panelPadding;

    //==========================================================================
    // Bypass button — top right
    // bypassButton.setBounds (w - Layout::bypassW - m,
    //                         m + (tH - Layout::bypassH) / 2,
    //                         Layout::bypassW,
    //                         Layout::bypassH);

    //==========================================================================
    // Resonator panel — two knobs centred horizontally
    {
        const int resonatorW = 200;
        const int panelCentreY = m + tH / 2;
        const int knobSpacing  = 8;
        const int totalW       = kS * 2 + knobSpacing;
        const int startX       = m + (resonatorW - totalW) / 2;
        const int knobY        = panelCentreY - kS / 2 - lH / 2 + Layout::titleHeight / 2;

        feedbackKnob.setBounds (startX,               knobY, kS, kS);
        feedbackLabel.setBounds(startX,               knobY + kS + 2, kS, lH);
        dampingKnob.setBounds  (startX + kS + knobSpacing, knobY, kS, kS);
        dampingLabel.setBounds (startX + kS + knobSpacing, knobY + kS + 2, kS, lH);
    }

    //==========================================================================
    // Bottom panel layout helpers
    const int numPanels = 4;
    const int panelW    = (w - m * (numPanels + 1)) / numPanels;
    const int panelY    = m + tH + m;

    auto knobRow = [&] (int panelX, int panelContentY, int count,
                        std::initializer_list<juce::Slider*> sliders,
                        std::initializer_list<juce::Label*>  labels)
    {
        const int totalKnobW = count * kS + (count - 1) * 6;
        int startX = panelX + (panelW - totalKnobW) / 2;
        auto sit = sliders.begin();
        auto lit = labels.begin();
        for (int i = 0; i < count; ++i)
        {
            (*sit)->setBounds (startX, panelContentY, kS, kS);
            (*lit)->setBounds (startX, panelContentY + kS + 2, kS, lH);
            startX += kS + 6;
            ++sit; ++lit;
        }
    };

    //==========================================================================
    // Trigger panel
    {
        const int px = m;
        const int contentY = panelY + Layout::titleHeight + pad;
        knobRow (px, contentY, 3,
                 { &thresholdKnob, &releaseKnob, &softnessKnob },
                 { &thresholdLabel, &releaseLabel, &softnessLabel });
    }

    //==========================================================================
    // Envelope panel
    {
        const int px        = m * 2 + panelW;
        const int available = bH - Layout::titleHeight - pad * 3 - kS - lH;
        const int displayH  = available;
        envelopeDisplay->setBounds (px + pad,
                                    panelY + Layout::titleHeight + pad,
                                    panelW - pad * 2,
                                    displayH);

        const int knobY = panelY + Layout::titleHeight + pad + displayH + pad;
        knobRow (px, knobY, 4,
                { &attackKnob, &decayKnob, &sustainKnob, &releaseEnvKnob },
                { &attackLabel, &decayLabel, &sustainLabel, &releaseEnvLabel });
    }

    //==========================================================================
    // Arpeggiator panel
    {
        const int px       = m * 3 + panelW * 2;
        const int displayH = bH / 3;
        arpDisplay->setBounds (px + pad,
                               panelY + Layout::titleHeight,
                               panelW - pad * 2,
                               displayH);

        const int row1Y = panelY + Layout::titleHeight + displayH + pad;
        knobRow (px, row1Y, 3,
                 { &rateKnob, &gateKnob, &modeKnob },
                 { &rateLabel, &gateLabel, &modeLabel });

        const int row2Y = row1Y + stride;
        knobRow (px, row2Y, 3,
                 { &deviationKnob, &scatterKnob, &octaveRangeKnob },
                 { &deviationLabel, &scatterLabel, &octaveRangeLabel });
    }

    //==========================================================================
    // Mixer panel
    {
        const int px        = m * 4 + panelW * 3;
        const int sliderH   = bH - Layout::titleHeight - stride - pad * 3;
        const int sliderW   = 36;
        const int sliderY   = panelY + Layout::titleHeight + pad;
        const int spacing   = 10;
        const int totalSliderW = sliderW * 2 + spacing;
        const int sliderStartX = px + (panelW - totalSliderW - kS - spacing) / 2;

        arpGainSlider.setBounds   (sliderStartX,               sliderY, sliderW, sliderH);
        arpGainLabel.setBounds    (sliderStartX,               sliderY + sliderH + 2, sliderW, lH);
        chordGainSlider.setBounds (sliderStartX + sliderW + spacing, sliderY, sliderW, sliderH);
        chordGainLabel.setBounds  (sliderStartX + sliderW + spacing, sliderY + sliderH + 2, sliderW, lH);

        const int mixX = sliderStartX + totalSliderW + spacing;
        const int mixY = sliderY + (sliderH - kS) / 2;
        mixKnob.setBounds  (mixX, mixY,     kS, kS);
        mixLabel.setBounds (mixX, mixY + kS + 2, kS, lH);
    }
}
