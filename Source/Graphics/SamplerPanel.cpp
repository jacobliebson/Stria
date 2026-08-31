#include "SamplerPanel.h"
#include "ResonatorPalette.h"

SamplerPanel::SamplerPanel (SamplerEngine& eng, double& sampleRateRef, juce::AudioProcessorValueTreeState& apvtsRef)
    : engine (eng), sampleRate (sampleRateRef), apvts (apvtsRef), waveformDisplay (eng)
{
    engine.addChangeListener (this);

    waveformDisplay.onFileDropped = [this] (juce::File f) { loadFile (f); };
    addAndMakeVisible (waveformDisplay);

    // Trim handle drags are reported one side at a time via these two
    // callbacks. Route each through the APVTS (SAMPLE_TRIM_START/END) so the
    // drag is both applied to the engine and persisted — the display no
    // longer touches the engine directly.
    waveformDisplay.onStartPointChanged = [this] (float v) { setTrimStartFromUI (v); };
    waveformDisplay.onEndPointChanged   = [this] (float v) { setTrimEndFromUI (v); };

    addAndMakeVisible (loopToggle);
    addAndMakeVisible (triggerToggle);
    addAndMakeVisible (startToggle);

    // Icon toggles already show their meaning on hover via tooltip; these
    // per-icon captions keep that meaning visible at a glance too, each
    // centred under its own button rather than as one combined line.
    loopToggle.onStateLabelChanged    = [this] { updateModeStateLabels(); };
    triggerToggle.onStateLabelChanged = [this] { updateModeStateLabels(); };
    startToggle.onStateLabelChanged   = [this] { updateModeStateLabels(); };

    for (auto* stateLabel : { &loopStateLabel, &triggerStateLabel, &startStateLabel })
    {
        stateLabel->setJustificationType (juce::Justification::centred);
        stateLabel->setColour (juce::Label::textColourId, ResonatorPalette::textSecondary());
        stateLabel->setFont (juce::Font (juce::FontOptions().withHeight (11.0f)));
        stateLabel->setMinimumHorizontalScale (0.7f);
        addAndMakeVisible (*stateLabel);
    }
    updateModeStateLabels();

    for (auto* groupLabel : { &sampleGroupLabel, &modeGroupLabel, &editGroupLabel })
    {
        groupLabel->setJustificationType (juce::Justification::centred);
        groupLabel->setColour (juce::Label::textColourId, ResonatorPalette::textSecondary().withAlpha (0.7f));
        groupLabel->setFont (juce::Font (juce::FontOptions().withHeight (10.0f)));
        addAndMakeVisible (*groupLabel);
    }
    sampleGroupLabel.setText ("SAMPLE", juce::dontSendNotification);
    modeGroupLabel.setText   ("MODE",   juce::dontSendNotification);
    editGroupLabel.setText   ("EDIT",   juce::dontSendNotification);

    addAndMakeVisible (reverseButton);

    trimButton.onClick = [this]
    {
        waveformDisplay.rebuildWaveformPath();
    };
    addAndMakeVisible (trimButton);

    resetButton.onClick = [this]
    {
        waveformDisplay.resetTrim();
        setTrimStartFromUI (0.0f); // goes through APVTS so it's persisted too
        setTrimEndFromUI (1.0f);
        waveformDisplay.rebuildWaveformPath();
    };
    addAndMakeVisible (resetButton);

    gainKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    gainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
    gainKnob.setTextValueSuffix (" x");
    addAndMakeVisible (gainKnob);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    pitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    pitchKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
    pitchKnob.setTextValueSuffix (" st");
    addAndMakeVisible (pitchKnob);

    pitchLabel.setText ("Pitch", juce::dontSendNotification);
    pitchLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (pitchLabel);

    fileNameLabel.setText ("No file loaded", juce::dontSendNotification);
    fileNameLabel.setJustificationType (juce::Justification::centredLeft);
    fileNameLabel.setColour (juce::Label::textColourId, ResonatorPalette::textSecondary());
    addAndMakeVisible (fileNameLabel);

    trimButton.setColour (juce::TextButton::buttonColourId,  ResonatorPalette::backgroundWidget());
    trimButton.setColour (juce::TextButton::textColourOffId,  ResonatorPalette::textSecondary());
    trimButton.setColour (juce::TextButton::textColourOnId,  ResonatorPalette::textSecondary());

    resetButton.setColour (juce::TextButton::buttonColourId,  ResonatorPalette::backgroundWidget());
    resetButton.setColour (juce::TextButton::textColourOffId,  ResonatorPalette::textSecondary());
    resetButton.setColour (juce::TextButton::textColourOnId,  ResonatorPalette::textSecondary());
    
    loadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser> (
            "Load Audio Sample",
            juce::File::getSpecialLocation (juce::File::userMusicDirectory),
            "*.wav;*.aiff;*.aif;*.mp3;*.flac;*.ogg");

        fileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                  | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      auto result = fc.getResult();
                                      if (result.existsAsFile())
                                          loadFile (result);
                                  });
    };
    addAndMakeVisible (loadButton);

    // Widget <-> APVTS sync: sets initial value/range from the parameter,
    // updates the parameter on user interaction, and updates the widget on
    // host automation or preset load.
    gainAttachment          = std::make_unique<SliderAttachment> (apvts, gainParamID,        gainKnob);
    pitchAttachment         = std::make_unique<SliderAttachment> (apvts, pitchParamID,       pitchKnob);
    loopAttachment          = std::make_unique<ButtonAttachment> (apvts, loopParamID,        loopToggle);
    freeRunAttachment       = std::make_unique<ButtonAttachment> (apvts, freeRunParamID,     triggerToggle);
    startRandomAttachment   = std::make_unique<ButtonAttachment> (apvts, startRandomParamID, startToggle);
    reverseAttachment       = std::make_unique<ButtonAttachment> (apvts, reverseParamID,     reverseButton);

    // APVTS -> engine sync, in one place, regardless of what caused the change.
    apvts.addParameterListener (gainParamID,        this);
    apvts.addParameterListener (pitchParamID,       this);
    apvts.addParameterListener (loopParamID,        this);
    apvts.addParameterListener (freeRunParamID,     this);
    apvts.addParameterListener (startRandomParamID, this);
    apvts.addParameterListener (reverseParamID,     this);
    apvts.addParameterListener (trimStartParamID,   this);
    apvts.addParameterListener (trimEndParamID,     this);

    // addParameterListener only fires on future changes, so push current
    // values into the engine now (covers a preset already loaded before
    // this panel existed).
    parameterChanged (gainParamID,        *apvts.getRawParameterValue (gainParamID));
    parameterChanged (pitchParamID,       *apvts.getRawParameterValue (pitchParamID));
    parameterChanged (loopParamID,        *apvts.getRawParameterValue (loopParamID));
    parameterChanged (freeRunParamID,     *apvts.getRawParameterValue (freeRunParamID));
    parameterChanged (startRandomParamID, *apvts.getRawParameterValue (startRandomParamID));
    parameterChanged (reverseParamID,     *apvts.getRawParameterValue (reverseParamID));
    parameterChanged (trimStartParamID,   *apvts.getRawParameterValue (trimStartParamID));
    parameterChanged (trimEndParamID,     *apvts.getRawParameterValue (trimEndParamID));

    updateControlStates();

    if (engine.hasSample())
    {
        waveformDisplay.sampleLoaded (engine.getBufferForDisplay());
        fileNameLabel.setText (engine.getLoadedFileName(), juce::dontSendNotification);
    }
}

SamplerPanel::~SamplerPanel()
{
    engine.removeChangeListener (this);

    apvts.removeParameterListener (gainParamID,        this);
    apvts.removeParameterListener (pitchParamID,       this);
    apvts.removeParameterListener (loopParamID,        this);
    apvts.removeParameterListener (freeRunParamID,     this);
    apvts.removeParameterListener (startRandomParamID, this);
    apvts.removeParameterListener (reverseParamID,     this);
    apvts.removeParameterListener (trimStartParamID,   this);
    apvts.removeParameterListener (trimEndParamID,     this);
}

//==============================================================================
void SamplerPanel::loadFile (const juce::File& file)
{
    if (engine.loadFile (file, sampleRate))
    {
        fileNameLabel.setText (file.getFileName(), juce::dontSendNotification);
        waveformDisplay.sampleLoaded (engine.getBufferForDisplay());
    }
}

void SamplerPanel::updateControlStates()
{
    // Key-triggered = triggerToggle off (false = "Free Run" state is inactive)
    const bool isKeyTriggered = ! engine.getFreeRun();

    triggerSourceButton.setEnabled (isKeyTriggered);

    // Start-at-start vs. random-start only matters once playback can
    // (re)start from a trigger, i.e. in key-trigger mode. In free-run mode
    // it only affects the one-time start position on load, so leave it
    // enabled either way — but it's most meaningful when key-triggered.
    startToggle.setAlpha (isKeyTriggered ? 1.0f : 0.55f);
}

void SamplerPanel::updateModeStateLabels()
{
    loopStateLabel.setText    (loopToggle.getCurrentStateLabel(),    juce::dontSendNotification);
    triggerStateLabel.setText (triggerToggle.getCurrentStateLabel(), juce::dontSendNotification);
    startStateLabel.setText   (startToggle.getCurrentStateLabel(),   juce::dontSendNotification);
}

//==============================================================================
void SamplerPanel::parameterChanged (const juce::String& parameterID, float newValue)
{
    // NB: can be called from the audio thread during host automation —
    // only touch the engine's lock-free setters here, and hop to the
    // message thread for anything UI-related.
    if (parameterID == gainParamID)
    {
        engine.setGain (newValue);
    }
    else if (parameterID == pitchParamID)
    {
        engine.setPitchSemitones (newValue);
    }
    else if (parameterID == loopParamID)
    {
        engine.setLoopEnabled (newValue >= 0.5f);
    }
    else if (parameterID == freeRunParamID)
    {
        engine.setFreeRun (newValue >= 0.5f);
        juce::MessageManager::callAsync ([this] { updateControlStates(); });
    }
    else if (parameterID == startRandomParamID)
    {
        engine.setStartRandom (newValue >= 0.5f);
    }
    else if (parameterID == reverseParamID)
    {
        engine.setReverse (newValue >= 0.5f);
    }
    else if (parameterID == trimStartParamID)
    {
        engine.setStartPoint (newValue);
    }
    else if (parameterID == trimEndParamID)
    {
        engine.setEndPoint (newValue);
    }
}

//==============================================================================
void SamplerPanel::setTrimStartFromUI (float startNormalised)
{
    // Routed through the APVTS (rather than calling engine.setStartPoint
    // directly) so the trim is both applied immediately and persisted as
    // part of the preset via SAMPLE_TRIM_START. parameterChanged() above
    // applies the value to the engine once the APVTS round-trips it back.
    if (auto* p = apvts.getParameter (trimStartParamID))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, startNormalised));
        p->endChangeGesture();
    }
}

void SamplerPanel::setTrimEndFromUI (float endNormalised)
{
    if (auto* p = apvts.getParameter (trimEndParamID))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, endNormalised));
        p->endChangeGesture();
    }
}

void SamplerPanel::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source != &engine)
        return;

    if (engine.hasSample())
    {
        waveformDisplay.sampleLoaded (engine.getBufferForDisplay());
        fileNameLabel.setText (engine.getLoadedFileName(), juce::dontSendNotification);
    }
    else
    {
        fileNameLabel.setText ("No file loaded", juce::dontSendNotification);
    }

    waveformDisplay.rebuildWaveformPath();
}


//==============================================================================
void SamplerPanel::paint (juce::Graphics& g)
{
    juce::Rectangle<float> b = getLocalBounds().toFloat();

    // Panel background
    g.setColour (ResonatorPalette::backgroundPanel());
    g.fillRoundedRectangle (b, 8.0f);

    // Panel border
    g.setColour (ResonatorPalette::borderPanel());
    g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

    // Thin dividers between the Sample / Mode / Edit clusters in the
    // bottom control strip, so the three groups read as distinct without
    // needing boxes or extra panels.
    if (controlStripBottom > controlStripTop)
    {
        g.setColour (ResonatorPalette::borderPanel());
        g.drawLine ((float) dividerX1, (float) controlStripTop, (float) dividerX1, (float) controlStripBottom, 1.0f);
        g.drawLine ((float) dividerX2, (float) controlStripTop, (float) dividerX2, (float) controlStripBottom, 1.0f);
    }
}


namespace SamplerPanelLayout
{
    // Outer margin
    constexpr int margin = 10;

    // Top row (file name + load button)
    constexpr int topRowHeight   = 24;
    constexpr int loadButtonW    = 90;
    constexpr int topRowGap      = 6;

    // Gap between top row and waveform
    constexpr int waveformTopGap = 6;

    // Bottom control strip is organised into three clusters — Sample
    // (gain/pitch), Mode (the three icon toggles), and Edit (reverse/trim/
    // reset) — each with its own small uppercase header, separated by thin
    // vertical dividers (drawn in paint()) instead of being one undivided
    // row of controls.
    constexpr int controlTopGap  = 6;
    constexpr int groupLabelH    = 12;   // "SAMPLE" / "MODE" / "EDIT" headers
    constexpr int rowGap         = 4;    // gap above/below the main control row
    constexpr int captionH       = 16;   // knob value labels / mode status line
    constexpr int groupGap       = 32;   // total horizontal gap between clusters

    // Knobs
    constexpr int knobW    = 64;
    constexpr int knobH    = 50;
    constexpr int labelH   = captionH;
    constexpr int knobGap  = 8;

    // Main row height is set by the tallest control (the knobs); icons and
    // toggle buttons are vertically centred within it.
    constexpr int mainRowH  = knobH;

    constexpr int controlHeight = groupLabelH + rowGap + mainRowH + rowGap + captionH;

    // Icon toggle group (Loop/One-shot, Key Trigger/Free Run, Start/Random).
    // Spaced out generously — there's room for it, and it keeps each icon's
    // hit target and status caption from crowding its neighbours.
    constexpr int iconSize = 32;
    constexpr int iconGap  = 34;

    // Reverse / Trim / Reset buttons
    constexpr int toggleH       = 22;
    constexpr int toggleSpacing = 8;
    constexpr int reverseW      = 60;
    constexpr int trimW         = 60;
    constexpr int resetW        = 60;
}

void SamplerPanel::resized()
{
    using namespace SamplerPanelLayout;

    auto bounds = getLocalBounds().reduced (margin);
    const int left  = bounds.getX();
    const int top   = bounds.getY();
    const int right = bounds.getRight();

    // ---- Top row: file name + load button ----
    const int topRowY = top;
    loadButton.setBounds (right - loadButtonW, topRowY, loadButtonW, topRowHeight);

    const int fileNameW = (right - loadButtonW - topRowGap) - left;
    fileNameLabel.setBounds (left, topRowY, fileNameW, topRowHeight);

    // ---- Waveform display ----
    const int waveformY = topRowY + topRowHeight + waveformTopGap;
    const int waveformH = bounds.getHeight() - topRowHeight - waveformTopGap
                                              - controlHeight - controlTopGap;
    waveformDisplay.setBounds (left, waveformY, bounds.getWidth(), waveformH);

    // ---- Bottom control strip: Sample | Mode | Edit ----
    const int stripY = waveformY + waveformH + controlTopGap;
    controlStripTop    = stripY;
    controlStripBottom = stripY + controlHeight;

    const int groupLabelY = stripY;
    const int mainRowY    = groupLabelY + groupLabelH + rowGap;
    const int captionY    = mainRowY + mainRowH + rowGap;

    int x = left;

    // -- Sample cluster: gain + pitch knobs --
    const int sampleGroupW = knobW * 2 + knobGap;
    sampleGroupLabel.setBounds (x, groupLabelY, sampleGroupW, groupLabelH);

    gainKnob.setBounds  (x, mainRowY, knobW, knobH);
    gainLabel.setBounds (x, captionY, knobW, labelH);
    x += knobW + knobGap;

    pitchKnob.setBounds  (x, mainRowY, knobW, knobH);
    pitchLabel.setBounds (x, captionY, knobW, labelH);
    x += knobW;

    x += groupGap / 2;
    dividerX1 = x;
    x += groupGap / 2;

    // -- Mode cluster: icon toggles, each with its own live caption
    // underneath spelling out what it currently means (backs up its
    // tooltip) — centred on that icon specifically, not the group as a
    // whole, so the caption reads as clearly belonging to its button.
    const int modeGroupW = iconSize * 3 + iconGap * 2;
    modeGroupLabel.setBounds (x, groupLabelY, modeGroupW, groupLabelH);

    const int iconY = mainRowY + (mainRowH - iconSize) / 2;

    // Each caption gets the icon's own width plus half the gap on either
    // side, so neighbouring captions can grow toward each other without
    // touching, and each stays centred under its own icon.
    const int capW = iconSize + iconGap - 6;

    const int loopX    = x;
    loopToggle.setBounds (loopX, iconY, iconSize, iconSize);
    loopStateLabel.setBounds (loopX + iconSize / 2 - capW / 2, captionY, capW, captionH);
    x += iconSize + iconGap;

    const int triggerX = x;
    triggerToggle.setBounds (triggerX, iconY, iconSize, iconSize);
    triggerStateLabel.setBounds (triggerX + iconSize / 2 - capW / 2, captionY, capW, captionH);
    x += iconSize + iconGap;

    const int startX = x;
    startToggle.setBounds (startX, iconY, iconSize, iconSize);
    startStateLabel.setBounds (startX + iconSize / 2 - capW / 2, captionY, capW, captionH);
    x += iconSize;

    x += groupGap / 2;
    dividerX2 = x;
    x += groupGap / 2;

    // -- Edit cluster: reverse / trim / reset --
    const int editGroupW = reverseW + trimW + resetW + toggleSpacing * 2;
    editGroupLabel.setBounds (x, groupLabelY, editGroupW, groupLabelH);

    const int toggleY = mainRowY + (mainRowH - toggleH) / 2;
    reverseButton.setBounds (x, toggleY, reverseW, toggleH);
    x += reverseW + toggleSpacing;

    trimButton.setBounds (x, toggleY, trimW, toggleH);
    x += trimW + toggleSpacing;

    resetButton.setBounds (x, toggleY, resetW, toggleH);
}