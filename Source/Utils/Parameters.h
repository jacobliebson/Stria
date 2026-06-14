#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

class Parameters {
    public:
        static juce::AudioProcessorValueTreeState::ParameterLayout configureParameters ();
};