project(AnalogEQ VERSION 1.0.4 LANGUAGES C CXX)

juce_add_plugin(AnalogEQ
    COMPANY_NAME "Starry"
    PLUGIN_MANUFACTURER_CODE Stry
    PLUGIN_CODE AneQ
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Analog EQ"

    VERSION 1.0.4
    BUNDLE_ID com.starrydsp.analogeq

    DESCRIPTION "Modular Analog-Style Graphic Equalizer with Interactive UI"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE

    VST3_CATEGORIES Fx EQ
    AU_MAIN_TYPE kAudioUnitType_Effect
)

target_sources(AnalogEQ
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/FilterBase.h
        Source/DSP/FilterTypes.h
        Source/DSP/FilterChain.h
        Source/DSP/FilterChain.cpp
        Source/UI/FrequencyResponseCurve.h
        Source/UI/FrequencyResponseCurve.cpp
        Source/UI/FilterNode.h
        Source/UI/FilterNode.cpp
        Source/UI/SpectrumAnalyzer.h
        Source/UI/SpectrumAnalyzer.cpp
        Source/UI/FilterControlPanel.h
        Source/UI/FilterControlPanel.cpp
        Source/Utils/ParameterHelper.h
        Source/Utils/ParameterHelper.cpp
)

target_include_directories(AnalogEQ
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(AnalogEQ
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(AnalogEQ
    PRIVATE
        ai_ui
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

