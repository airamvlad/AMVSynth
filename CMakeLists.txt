cmake_minimum_required(VERSION 3.22)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(AMVSynth VERSION 0.11.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(AMV_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND AMV_FORMATS AU)
endif()

juce_add_plugin(AMVSynth
    COMPANY_NAME "AMV"
    PLUGIN_MANUFACTURER_CODE Amvs
    PLUGIN_CODE Amv1
    FORMATS ${AMV_FORMATS}
    PRODUCT_NAME "AMVSynth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth)

target_sources(AMVSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(AMVSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(AMVSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

option(AMV_BUILD_TESTS "Build the offline engine tests" ON)
if(AMV_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(AMVSynthTests PRODUCT_NAME "AMVSynthTests")
    target_sources(AMVSynthTests PRIVATE tests/EngineTests.cpp)
    target_include_directories(AMVSynthTests PRIVATE Source)
    target_compile_definitions(AMVSynthTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(AMVSynthTests
        PRIVATE
            juce::juce_audio_basics
            juce::juce_audio_processors
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
    add_test(NAME EngineTests COMMAND AMVSynthTests)

    # Plays the factory presets through the plugin itself.
    juce_add_console_app(AMVSynthPresetTests PRODUCT_NAME "AMVSynthPresetTests")
    target_sources(AMVSynthPresetTests PRIVATE tests/PresetTests.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(AMVSynthPresetTests PRIVATE Source)
    target_compile_definitions(AMVSynthPresetTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_VersionString="${PROJECT_VERSION}")
    target_link_libraries(AMVSynthPresetTests
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_recommended_config_flags)
    add_test(NAME PresetTests COMMAND AMVSynthPresetTests)
endif()
