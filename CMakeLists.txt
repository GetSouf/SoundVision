cmake_minimum_required(VERSION 3.22)

project(SoundVision VERSION 0.2.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Allow overriding JUCE path: cmake -DJUCE_PATH=C:/dev/JUCE ...
set(JUCE_PATH "C:/dev/JUCE" CACHE PATH "Path to JUCE")

if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}. Pass -DJUCE_PATH=...")
endif()

add_subdirectory(${JUCE_PATH} JUCE)

juce_add_plugin(SoundVision
    COMPANY_NAME "SoundVision"
    BUNDLE_ID "com.soundvision.plugin"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE SVis
    PLUGIN_CODE Sv01
    FORMATS VST3 Standalone
    PRODUCT_NAME "SoundVision"
    VST3_CATEGORIES "Fx|Analyzer"
    DESCRIPTION "Spatial multi-track sound visualizer for mixer buses")

juce_generate_juce_header(SoundVision)

target_sources(SoundVision
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp/BandFilter.cpp
        Source/dsp/StereoAnalyzer.cpp
        Source/dsp/ParticleSystem.cpp
        Source/ipc/SoundVisionHub.cpp
        Source/ui/SpatialView.cpp
        Source/ui/SourceLegend.cpp
        Source/ui/SoundVisionLookAndFeel.cpp
        Source/params/PluginParameters.cpp)

target_compile_definitions(SoundVision
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0)

target_include_directories(SoundVision
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_link_libraries(SoundVision
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(MSVC)
    target_compile_options(SoundVision PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
endif()
