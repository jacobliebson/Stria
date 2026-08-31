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

    // Bottom control strip
    constexpr int controlHeight  = 80;
    constexpr int controlTopGap  = 6;

    // Knobs
    constexpr int knobW    = 64;
    constexpr int knobH    = 50;
    constexpr int labelH   = 16;
    constexpr int knobGap  = 8;

    // Icon toggle group (replaces the old mode dropdown)
    constexpr int modeGap        = 16;   // gap before the toggle group starts
    constexpr int iconSize       = 32;
    constexpr int iconGap        = 10;

    // Toggles
    constexpr int toggleGap      = 16;   // gap before the reverse/trim/reset group starts
    constexpr int toggleH        = 22;
    constexpr int toggleSpacing  = 8;    // gap between toggle buttons
    constexpr int reverseW       = 60;
    constexpr int trimW          = 60;
    constexpr int resetW         = 60;
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

    // ---- Bottom control row ----
    const int controlY = waveformY + waveformH + controlTopGap;
    int x = left;

    // Gain knob
    gainKnob.setBounds  (x, controlY, knobW, knobH);
    gainLabel.setBounds (x, controlY + knobH, knobW, labelH);
    x += knobW + knobGap;

    // Pitch knob
    pitchKnob.setBounds  (x, controlY, knobW, knobH);
    pitchLabel.setBounds (x, controlY + knobH, knobW, labelH);
    x += knobW + modeGap;
    int x2 = x;

    // Icon toggle group — vertically centred against the knob height above
    const int iconY = controlY + (knobH - iconSize) / 2;
    loopToggle.setBounds    (x, iconY, iconSize, iconSize);
    x += iconSize + iconGap;
    triggerToggle.setBounds (x, iconY, iconSize, iconSize);
    x += iconSize + iconGap;
    startToggle.setBounds   (x, iconY, iconSize, iconSize);

    // Toggle buttons — trigger-source button removed, reverse/trim/reset remain
    const int toggleY = iconY + iconSize + toggleSpacing;
    x = x2;

    reverseButton.setBounds (x, toggleY, reverseW, toggleH);
    x += reverseW + toggleSpacing;

    trimButton.setBounds (x, toggleY, trimW, toggleH);
    x += trimW + toggleSpacing;

    resetButton.setBounds (x, toggleY, resetW, toggleH);
}