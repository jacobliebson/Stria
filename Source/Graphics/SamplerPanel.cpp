#include "SamplerPanel.h"

SamplerPanel::SamplerPanel (SamplerEngine& eng, double& sampleRateRef, juce::AudioProcessorValueTreeState& apvtsRef)
    : engine (eng), sampleRate (sampleRateRef), apvts (apvtsRef), waveformDisplay (eng)
{
    engine.addChangeListener (this);

    waveformDisplay.onFileDropped = [this] (juce::File f) { loadFile (f); };
    addAndMakeVisible (waveformDisplay);

    // If/when SamplerWaveformDisplay grows drag-to-trim handles, wire it here
    // so trim changes flow through the APVTS like every other parameter:
    //     waveformDisplay.onTrimChanged = [this] (float s, float e) { setTrimFromUI (s, e); };

    playbackModeBox.addItem ("Continuous Loop",        1);
    playbackModeBox.addItem ("Key Trigger - Start",    2);
    playbackModeBox.addItem ("Key Trigger - Random",   3);
    addAndMakeVisible (playbackModeBox);

    playbackModeLabel.setText ("Mode", juce::dontSendNotification);
    playbackModeLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (playbackModeLabel);

    addAndMakeVisible (reverseButton);

    trimButton.onClick = [this]
    {
        waveformDisplay.rebuildWaveformPath();
    };
    addAndMakeVisible (trimButton);

    resetButton.onClick = [this]
    {
        waveformDisplay.resetTrim();
        engine.setStartPoint(0.0f);
        engine.setEndPoint(1.0f);
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
    gainAttachment         = std::make_unique<SliderAttachment>   (apvts, gainParamID,         gainKnob);
    pitchAttachment        = std::make_unique<SliderAttachment>   (apvts, pitchParamID,        pitchKnob);
    playbackModeAttachment = std::make_unique<ComboBoxAttachment> (apvts, playbackModeParamID, playbackModeBox);
    reverseAttachment      = std::make_unique<ButtonAttachment>   (apvts, reverseParamID,      reverseButton);

    // APVTS -> engine sync, in one place, regardless of what caused the change.
    apvts.addParameterListener (gainParamID,         this);
    apvts.addParameterListener (pitchParamID,        this);
    apvts.addParameterListener (playbackModeParamID, this);
    apvts.addParameterListener (reverseParamID,      this);

    // addParameterListener only fires on future changes, so push current
    // values into the engine now (covers a preset already loaded before
    // this panel existed).
    parameterChanged (gainParamID,         *apvts.getRawParameterValue (gainParamID));
    parameterChanged (pitchParamID,        *apvts.getRawParameterValue (pitchParamID));
    parameterChanged (playbackModeParamID, *apvts.getRawParameterValue (playbackModeParamID));
    parameterChanged (reverseParamID,      *apvts.getRawParameterValue (reverseParamID));

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

    apvts.removeParameterListener (gainParamID,         this);
    apvts.removeParameterListener (pitchParamID,        this);
    apvts.removeParameterListener (playbackModeParamID, this);
    apvts.removeParameterListener (reverseParamID,      this);
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
    const bool isKeyTriggered =
        (engine.getPlaybackMode() != SamplerEngine::PlaybackMode::ContinuousLoop);

    triggerSourceButton.setEnabled (isKeyTriggered);
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
    else if (parameterID == playbackModeParamID)
    {
        switch (static_cast<int> (newValue))
        {
            case 0: engine.setPlaybackMode (SamplerEngine::PlaybackMode::ContinuousLoop);       break;
            case 1: engine.setPlaybackMode (SamplerEngine::PlaybackMode::KeyTriggerFromStart);  break;
            case 2: engine.setPlaybackMode (SamplerEngine::PlaybackMode::KeyTriggerFromRandom); break;
            default: break;
        }

        juce::MessageManager::callAsync ([this] { updateControlStates(); });
    }
    else if (parameterID == reverseParamID)
    {
        engine.setReverse (newValue >= 0.5f);
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
    g.fillAll (ResonatorPalette::backgroundPanel());
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

    // Mode selector
    constexpr int modeGap        = 16;
    constexpr int modeLabelW     = 40;
    constexpr int modeBoxW       = 180;
    constexpr int modeControlH   = 24;

    // Toggles
    constexpr int toggleGap      = 16;   // gap before the toggle group starts
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

    // Mode selector
    const int modeControlY = controlY;
    playbackModeLabel.setBounds (x, modeControlY, modeLabelW, modeControlH);
    x += modeLabelW;
    playbackModeBox.setBounds (x, modeControlY, modeBoxW, modeControlH);
    x += modeBoxW + toggleGap;

    // Toggle buttons — trigger-source button removed, reverse/trim/reset remain
    const int toggleY = controlY + modeControlH + toggleSpacing;
    x = x2; 

    reverseButton.setBounds (x, toggleY, reverseW, toggleH);
    x += reverseW + toggleSpacing;

    trimButton.setBounds (x, toggleY, trimW, toggleH);
    x += trimW + toggleSpacing;

    resetButton.setBounds (x, toggleY, resetW, toggleH);
}