cmake_minimum_required(VERSION 3.22)

# Mac: build universale (Apple Silicon + Intel)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

project(VocalRiser VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Su Mac copia automaticamente AU/VST3 in ~/Library/Audio/Plug-Ins dopo la build
if(APPLE)
    set(VR_COPY_AFTER_BUILD TRUE)
else()
    set(VR_COPY_AFTER_BUILD FALSE)
endif()

juce_add_plugin(VocalRiser
    COMPANY_NAME "chef catta dsp"
    BUNDLE_ID "com.chefcattadsp.vocalriser"
    PLUGIN_MANUFACTURER_CODE Vrsr
    PLUGIN_CODE Vrs1
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Vocal Riser"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${VR_COPY_AFTER_BUILD})

target_sources(VocalRiser PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(VocalRiser PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(VocalRiser
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
