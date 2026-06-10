// Source/PluginEditor.cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ResonatorAnalyzer.h"
#include "ResonatorPalette.h"
#include <memory>
#include <string_view>

//==============================================================================
// Layout constants
namespace Layout
{
    constexpr int margin        = 12;
    constexpr int windowW   = 920;
    constexpr int windowH   = 742;  // reduced from 620 - 480
    constexpr int headerH   = 80;
    constexpr int topH      = 325;  // reduced from 140
    constexpr int bottomH   = windowH - topH - headerH - margin * 3;
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

    // Analyzer
    resonatorAnalyzer = std::make_unique<ResonatorAnalyzer>(audioProcessor);
    addAndMakeVisible (*resonatorAnalyzer);

    // Resonator
    setupKnob (feedbackKnob, feedbackLabel, "Feedback",
               feedbackAttachment, "FEEDBACK");
    setupKnob (dampingKnob,  dampingLabel,  "Damping",
               dampingAttachment,  "DAMPING");
    setupKnob (detuneKnob,  detuneLabel,  "Detune",
               detuneAttachment,  "DETUNE");
    setupDiscreteKnob(detuneModeKnob, detuneModeLabel, "Mode",
                detuneModeAttachment, "DETUNE_MODE");
    

    // Trigger
    setupKnob (thresholdKnob, thresholdLabel, "Threshold",
               thresholdAttachment, "TRIG_THRESHOLD", " dB");
    setupKnob (releaseKnob,   releaseLabel,   "Release",
               releaseAttachment,   "TRIG_RELEASE", " ms");
    setupKnob (softnessKnob,  softnessLabel,  "Attack",
               softnessAttachment,  "TRIG_SOFTNESS", " ms");

    // Envelope
    envelopeDisplay = std::make_unique<EnvelopeDisplay> (apvts,
                          "CHORD_ENV_ATTACK", "CHORD_ENV_DECAY", "CHORD_ENV_SUSTAIN", "CHORD_ENV_RELEASE");
    addAndMakeVisible (*envelopeDisplay);

    // Tab buttons — default to Chord
    chordEnvButton.setClickingTogglesState (false);
    arpEnvButton.setClickingTogglesState   (false);
    chordEnvButton.onClick = [this] { switchEnvelopeTo (false); };
    arpEnvButton.onClick   = [this] { switchEnvelopeTo (true);  };
    addAndMakeVisible (chordEnvButton);
    addAndMakeVisible (arpEnvButton);

    setupKnob (attackKnob,     attackLabel,     "Attack",
               attackAttachment,     "CHORD_ENV_ATTACK", " s");
    setupKnob (decayKnob,      decayLabel,      "Decay",
               decayAttachment,      "CHORD_ENV_DECAY", " s");
    setupKnob (sustainKnob,    sustainLabel,    "Sustain",
               sustainAttachment,    "CHORD_ENV_SUSTAIN", "%");
    setupKnob (releaseEnvKnob, releaseEnvLabel, "Release",
               releaseEnvAttachment, "CHORD_ENV_RELEASE", " s");

    // Arpeggiator
    arpDisplay = std::make_unique<ArpDisplay> (apvts,
                     "ARP_RATE", "ARP_GATE", "ARP_MODE",
                     "ARP_SCATTER", "ARP_RANGE");
    addAndMakeVisible (*arpDisplay);

    setupDiscreteKnob (rateKnob,       rateLabel,       "Rate",
                   rateAttachment,       "ARP_RATE");
    setupKnob         (gateKnob,       gateLabel,       "Gate",
                       gateAttachment,       "ARP_GATE", "%");
    setupDiscreteKnob (modeKnob,       modeLabel,       "Mode",
                       modeAttachment,       "ARP_MODE");
    setupKnob         (deviationKnob,  deviationLabel,  "Deviation",
                       deviationAttachment,  "ARP_DEVIATION", "%");
    setupKnob         (scatterKnob,    scatterLabel,    "Scatter",
                       scatterAttachment,    "ARP_SCATTER", "%");
    setupDiscreteKnob (octaveRangeKnob, octaveRangeLabel, "Range",
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

    setupKnob (mixKnob, mixLabel, "Mix", mixAttachment, "MIX", "%");

    // Bypass
    //bypassButton.setButtonText ("Bypass");
    //addAndMakeVisible (bypassButton);
    //bypassAttachment = std::make_unique<ButtonAttachment> (apvts, "ARP_BYPASS", bypassButton);

    // Initialise tab button and knob colours — Chord selected by default
    chordEnvButton.setColour (juce::TextButton::buttonColourId,  ResonatorPalette::accentPrimary().withAlpha(0.3f));
    chordEnvButton.setColour (juce::TextButton::textColourOnId,  ResonatorPalette::textPrimary());
    chordEnvButton.setColour (juce::TextButton::textColourOffId, ResonatorPalette::textPrimary());
    arpEnvButton.setColour   (juce::TextButton::buttonColourId,  ResonatorPalette::backgroundWidget());
    arpEnvButton.setColour   (juce::TextButton::textColourOnId,  ResonatorPalette::textPrimary());
    arpEnvButton.setColour   (juce::TextButton::textColourOffId, ResonatorPalette::textPrimary());
    chordEnvButton.setColour (juce::ComboBox::outlineColourId, ResonatorPalette::borderPanel());
    arpEnvButton.setColour   (juce::ComboBox::outlineColourId, ResonatorPalette::borderPanel());

    for (auto* knob : { &attackKnob, &decayKnob, &sustainKnob, &releaseEnvKnob })
        knob->setColour (juce::Slider::rotarySliderFillColourId, ResonatorPalette::accentPrimary());

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
                                                   const juce::String& paramId,
                                                   std::string suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 14);
    addAndMakeVisible (slider);
    slider.setTextValueSuffix(suffix);
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

void AudioPluginAudioProcessorEditor::switchEnvelopeTo (bool showArp)
{
    showingArpEnv = showArp;

    const juce::String  prefix     = showArp ? "ARP_ENV_" : "CHORD_ENV_";
    const juce::Colour  knobColour = showArp ? ResonatorPalette::accentSecondary()
                                             : ResonatorPalette::accentPrimary();

    // Swap display listeners and accent colour
    envelopeDisplay->setParameters  (prefix + "ATTACK", prefix + "DECAY",
                                     prefix + "SUSTAIN", prefix + "RELEASE");
    envelopeDisplay->setAccentColour (knobColour);

    // Destroy and recreate attachments pointing at the new parameter set
    attackAttachment.reset();
    decayAttachment.reset();
    sustainAttachment.reset();
    releaseEnvAttachment.reset();

    auto& apvts = audioProcessor.apvts;
    attackAttachment     = std::make_unique<SliderAttachment> (apvts, prefix + "ATTACK",  attackKnob);
    decayAttachment      = std::make_unique<SliderAttachment> (apvts, prefix + "DECAY",   decayKnob);
    sustainAttachment    = std::make_unique<SliderAttachment> (apvts, prefix + "SUSTAIN", sustainKnob);
    releaseEnvAttachment = std::make_unique<SliderAttachment> (apvts, prefix + "RELEASE", releaseEnvKnob);

    // Update knob arc colours
    for (auto* knob : { &attackKnob, &decayKnob, &sustainKnob, &releaseEnvKnob })
        knob->setColour (juce::Slider::rotarySliderFillColourId, knobColour);

    // Update button colours
    chordEnvButton.setColour (juce::TextButton::buttonColourId,
                              showArp ? ResonatorPalette::backgroundWidget()
                                      : ResonatorPalette::accentPrimary().withAlpha(0.3f));
    arpEnvButton.setColour   (juce::TextButton::buttonColourId,
                              showArp ? ResonatorPalette::accentSecondary().withAlpha(0.3f)
                                      : ResonatorPalette::backgroundWidget());
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
    g.setFont (juce::Font (juce::FontOptions().withHeight (11.0f)));
    g.setColour(ResonatorPalette::textSecondary());
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
    const int hH = Layout::headerH;
    const int tH = Layout::topH;
    const int w  = getWidth();

    // Title block
    juce::ColourGradient gradient(
        ResonatorPalette::textSecondary(),
        (float)0, (float)0, 
        ResonatorPalette::textSecondary().darker(0.3f), 
        (float)0, (float)hH, 
        false
    );

    g.setGradientFill(gradient);
    juce::FontOptions options = juce::FontOptions().withHeight (Layout::headerH);
    g.setFont (juce::Font (options));

    g.drawText ("S T R I A",
                m, 0,
                w - m * 2,
                hH,
                juce::Justification::centred);

    // Resonator panel (top left)
    const int resonatorW = 80;
    drawPanel (g, { m, m + hH, resonatorW, tH }, "Resonator");

    // Bottom panels — equal width
    const int numPanels  = 4;
    const int panelW     = (w - m * (numPanels + 1)) / numPanels;
    const int panelY     = m + hH + m + tH;

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
    const int hH     = Layout::headerH;
    const int tH     = Layout::topH;
    const int bH     = Layout::bottomH;
    const int w      = getWidth();
    const int kS     = Layout::knobSize;
    const int lH     = Layout::labelHeight;
    const int stride = Layout::knobStride;
    const int pad    = Layout::panelPadding;

    //==========================================================================
    // Analyzer panel
    {
        const int resonatorW   = 80;
        const int analyzerX    = m + resonatorW + m;
        const int analyzerY    = m + hH;
        const int analyzerW    = getWidth() - analyzerX - m;
        const int analyzerH    = tH;
        resonatorAnalyzer->setBounds (analyzerX, analyzerY, analyzerW, analyzerH);
    }
    // Resonator panel — four knobs stacked vertically
    {
        const int resonatorW = 80; // Narrower width for vertical stack
        const int panelCentreX = m + resonatorW / 2;
        const int knobSpacing  = 20; // Vertical space between groups
        const int labelSpace   = 20; // Space for labels
        
        // Starting Y position at the top of the panel
        int currentY = m + hH + m + Layout::titleHeight;

        // Helper to position a knob and its label
        auto setKnobBounds = [&](juce::Slider& knob, juce::Label& label) {
            knob.setBounds (panelCentreX - kS / 2, currentY, kS, kS);
            label.setBounds(panelCentreX - kS / 2, currentY + kS, kS, lH);
            currentY += kS + labelSpace;
        };

        setKnobBounds(feedbackKnob, feedbackLabel);
        setKnobBounds(dampingKnob,  dampingLabel);
        setKnobBounds(detuneKnob,   detuneLabel);
        setKnobBounds(detuneModeKnob,     detuneModeLabel);
    }

    //==========================================================================
    
    // Bottom panel layout helpers
    const int numPanels = 4;
    const int panelW    = (w - m * (numPanels + 1)) / numPanels;
    const int panelY    = m + hH + m + tH + m;

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
        const int px   = m * 2 + panelW;
        const int tabH = 12;
        const int tabW = 30;
        const int tabY = panelY + pad;

        chordEnvButton.setBounds (px + panelW - 2 * (tabW + 6),            panelY + 6, tabW, tabH);
        arpEnvButton.setBounds   (px + panelW - tabW - 6, panelY + 6, tabW, tabH);

        const int available = bH - Layout::titleHeight - pad * 3 - kS - lH;
        envelopeDisplay->setBounds (px + pad,
                                    panelY + Layout::titleHeight,
                                    panelW - pad * 2,
                                    available);

        const int knobY = panelY + Layout::titleHeight + bH / 3 + pad + stride;
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
