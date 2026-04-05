cmake_minimum_required(VERSION 3.15)

project(ai_vst_suite VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Shared dependencies
add_subdirectory(JUCE)

# Shared libraries and plugin projects
add_subdirectory(libs/ui)
add_subdirectory(plugins)
