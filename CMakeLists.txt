cmake_minimum_required(VERSION 3.22)

# Define your project
project(AUDIO_PLUGIN_TEMPLATE VERSION 1.0.0 LANGUAGES C CXX)

#0. Resolve intellisense desync
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. Enable CPM (CMake Package Manager) or FetchContent to grab JUCE
add_subdirectory(JUCE)

# 2. Create the JUCE Plugin Target
juce_add_plugin(MyJucePlugin
    # Set your unique Plugin Code and Manufacturer Code (4 characters each)
    PLUGIN_MANUFACTURER_CODE    Abcd
    PLUGIN_CODE                1234
    
    # Formats to build (VST3, AU, Standalone, etc.)
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "My Juce Plugin"
    COPY_PLUGIN_AFTER_BUILD TRUE
)



# 3. Add Source Files
target_sources(MyJucePlugin PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

# 4. Compile Features and Definitions
target_compile_features(MyJucePlugin PRIVATE cxx_std_20)

target_compile_definitions(MyJucePlugin PRIVATE
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

# 5. Link JUCE Modules
target_link_libraries(MyJucePlugin PRIVATE
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
