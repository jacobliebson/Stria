#pragma once
#include "PluginProcessor.h"

class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         public juce::Timer // 1. Add Timer Inheritance
{
public:
    AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    // 2. Add the timer callback declaration
    void timerCallback() override;

private:
    AudioPluginAudioProcessor& audioProcessor;
    
    // 3. Keep a local copy of the notes to check for visual changes


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};