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
    playbackModeBox.addItem ("Key Trigger — Start",    2);
    playbackModeBox.addItem ("Key Trigger — Random",   3);
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

    // Trigger source
    triggerSourceButton.setToggleState (engine.getTriggerFromRawMidi(),
                                        juce::dontSendNotification);
    triggerSourceButton.onClick = [this]
    {
        engine.setTriggerSource (triggerSourceButton.getToggleState());
    };
    addAndMakeVisible (triggerSourceButton);

    // Reverse
    reverseButton.setToggleState (engine.getReverse(), juce::dontSendNotification);
    reverseButton.onClick = [this]
    {
        engine.setReverse (reverseButton.getToggleState());
    };
    addAndMakeVisible (reverseButton);

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

void SamplerPanel::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    // File name and load button across the top
    const int topH   = 24;
    auto topRow      = bounds.removeFromTop (topH);
    loadButton.setBounds    (topRow.removeFromRight (90));
    topRow.removeFromRight  (6);
    fileNameLabel.setBounds (topRow);

    bounds.removeFromTop (6);

    // Waveform display takes the bulk of the space
    const int controlH = 80;
    auto waveformArea  = bounds.removeFromTop (bounds.getHeight() - controlH - 6);
    waveformDisplay.setBounds (waveformArea);

    bounds.removeFromTop (6);

    // Controls row at the bottom
    auto controlRow = bounds;

    const int knobW  = 64;
    const int knobH  = 50;
    const int labelH = 16;

    // Gain knob
    gainKnob.setBounds   (controlRow.removeFromLeft (knobW).removeFromTop (knobH));
    gainLabel.setBounds  (juce::Rectangle<int> (controlRow.getX() - knobW,
                                                 gainKnob.getBottom(), knobW, labelH));

    controlRow.removeFromLeft (8);

    // Pitch knob
    pitchKnob.setBounds  (controlRow.removeFromLeft (knobW).removeFromTop (knobH));
    pitchLabel.setBounds (juce::Rectangle<int> (controlRow.getX() - knobW,
                                                 pitchKnob.getBottom(), knobW, labelH));

    controlRow.removeFromLeft (16);

    // Mode selector
    const int modeW = 180;
    playbackModeLabel.setBounds (controlRow.removeFromLeft (40).withHeight (24).withCentre (
        juce::Point<int> (controlRow.getX() - 20, controlRow.getCentreY())));
    playbackModeBox.setBounds (controlRow.removeFromLeft (modeW).withHeight (24)
                                          .withY (controlRow.getCentreY() - 12));

    controlRow.removeFromLeft (16);

    // Toggle buttons stacked
    const int toggleH = 22;
    const int toggleW = 160;
    triggerSourceButton.setBounds (controlRow.removeFromLeft (toggleW).removeFromTop (toggleH));
    controlRow.removeFromLeft (8);
    reverseButton.setBounds (controlRow.removeFromLeft (80).removeFromTop (toggleH));
}

