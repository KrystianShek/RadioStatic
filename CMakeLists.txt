cmake_minimum_required(VERSION 3.22)
project(RadioStatic VERSION 1.0.0)

# JUCE (Version 8) ist als Git-Submodule unter "JUCE/" eingebunden:
#   git submodule update --init
add_subdirectory(JUCE)

juce_add_plugin(RadioStatic
    COMPANY_NAME "Krystian Shek"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Kshk
    PLUGIN_CODE Rdst
    FORMATS VST3 Standalone
    PRODUCT_NAME "RadioStatic")

# Eingebettete Sprachaufnahme (wird als BinaryData ins Plugin einkompiliert)
juce_add_binary_data(RadioStaticVoices SOURCES Resources/voice.wav)

juce_generate_juce_header(RadioStatic)

target_sources(RadioStatic PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/UiComponents.h)

target_compile_definitions(RadioStatic PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(RadioStatic
    PRIVATE
        RadioStaticVoices
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
