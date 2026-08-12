// Source/Graphics/Displays/SamplerPanel.cpp
#include "SamplerPanel.h"

SamplerPanel::SamplerPanel (SamplerEngine& eng, double& sampleRateRef)
    : engine (eng), sampleRate (sampleRateRef), waveformDisplay (eng)
{
    // Waveform display — file drop wired to loadFile
    waveformDisplay.onFileDropped = [this] (juce::File f) { loadFile (f); };
    addAndMakeVisible (waveformDisplay);

    // Playback mode
    playbackModeBox.addItem ("Continuous Loop",        1);
    playbackModeBox.addItem ("Key Trigger - Start",    2);
    playbackModeBox.addItem ("Key Trigger - Random",   3);
    playbackModeBox.setSelectedId (1, juce::dontSendNotification);
    playbackModeBox.onChange = [this]
    {
        switch (playbackModeBox.getSelectedId())
        {
            case 1: engine.setPlaybackMode (SamplerEngine::PlaybackMode::ContinuousLoop);      break;
            case 2: engine.setPlaybackMode (SamplerEngine::PlaybackMode::KeyTriggerFromStart); break;
            case 3: engine.setPlaybackMode (SamplerEngine::PlaybackMode::KeyTriggerFromRandom);break;
            default: break;
        }
        updateControlStates();
    };
    addAndMakeVisible (playbackModeBox);

    playbackModeLabel.setText ("Mode", juce::dontSendNotification);
    playbackModeLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (playbackModeLabel);

    // Reverse
    reverseButton.setToggleState (engine.getReverse(), juce::dontSendNotification);
    reverseButton.onClick = [this]
    {
        engine.setReverse (reverseButton.getToggleState());
    };
    addAndMakeVisible (reverseButton);

    // Trim
    trimButton.onClick = [this]->void
    {
        waveformDisplay.rebuildWaveformPath();
    };
    addAndMakeVisible(trimButton);

    // Reset
    resetButton.onClick = [this]->void
    {
        engine.setStartPoint(0.0f);
        engine.setEndPoint(1.0f);
        waveformDisplay.rebuildWaveformPath();
    };
    addAndMakeVisible(resetButton);

    // Gain knob
    gainKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    gainKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
    gainKnob.setRange (0.0, 2.0, 0.01);
    gainKnob.setValue (engine.getGain(), juce::dontSendNotification);
    gainKnob.setTextValueSuffix (" x");
    gainKnob.onValueChange = [this]
    {
        engine.setGain (static_cast<float> (gainKnob.getValue()));
    };
    addAndMakeVisible (gainKnob);

    gainLabel.setText ("Gain", juce::dontSendNotification);
    gainLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (gainLabel);

    // Pitch knob
    pitchKnob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    pitchKnob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 50, 14);
    pitchKnob.setRange (-24.0, 24.0, 0.1);
    pitchKnob.setValue (engine.getPitchSemitones(), juce::dontSendNotification);
    pitchKnob.setTextValueSuffix (" st");
    pitchKnob.onValueChange = [this]
    {
        engine.setPitchSemitones (static_cast<float> (pitchKnob.getValue()));
    };
    addAndMakeVisible (pitchKnob);

    pitchLabel.setText ("Pitch", juce::dontSendNotification);
    pitchLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (pitchLabel);

    // File name
    fileNameLabel.setText ("No file loaded", juce::dontSendNotification);
    fileNameLabel.setJustificationType (juce::Justification::centredLeft);
    fileNameLabel.setColour (juce::Label::textColourId, ResonatorPalette::textSecondary());
    addAndMakeVisible (fileNameLabel);

    // Load button
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

    updateControlStates();

    if (engine.hasSample())
    {
        waveformDisplay.sampleLoaded (engine.getBufferForDisplay());
        fileNameLabel.setText (engine.getLoadedFileName(), juce::dontSendNotification);
    }
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