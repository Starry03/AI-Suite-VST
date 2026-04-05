add_library(ai_ui STATIC)
set_target_properties(ai_ui PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Keep headers visible in IDEs and source groups.
target_sources(ai_ui
    PUBLIC
        include/ai_ui/ModernLookAndFeel.h
        include/ai_ui/ParameterTooltip.h
        include/ai_ui/DBMeter.h
    PRIVATE
        src/ModernLookAndFeel.cpp
        src/ParameterTooltip.cpp
        src/DBMeter.cpp
)

target_include_directories(ai_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(ai_ui
    PUBLIC
        juce::juce_gui_basics
)

target_compile_features(ai_ui PUBLIC cxx_std_17)



