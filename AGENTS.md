# AGENTS.md

Guidance for AI coding agents working in this repository.

## Project Overview

AI VST Suite is a C++17 JUCE/CMake workspace for audio plugins. The root project wires together the JUCE submodule, shared libraries, and plugin projects.

- `JUCE/`: JUCE submodule. Treat as third-party code; do not edit unless explicitly asked.
- `libs/ui/`: shared JUCE UI library, built as the `ai_ui` static library.
- `plugins/`: plugin workspace.
- `plugins/AnalogEQ/`: Analog EQ plugin, built with `juce_add_plugin`.

The main active plugin target is `AnalogEQ`, with generated format targets such as `AnalogEQ_VST3`.

## Build Commands

Run from the repository root:

```bash
cmake -S . -B build
cmake --build build --target AnalogEQ_VST3
```

Other useful targets depend on the generator and host platform, but `AnalogEQ` and the JUCE-generated plugin format targets are the expected entry points.

## Coding Conventions

- Use C++17 and existing JUCE idioms.
- Keep plugin source under `plugins/<PluginName>/Source`.
- Keep reusable UI components under `libs/ui/include/ai_ui` and `libs/ui/src`.
- Add new source/header files to the relevant `target_sources(...)` block in CMake.
- Prefer clear ownership and RAII. Avoid raw owning pointers.
- Preserve existing class and file naming style.
- Comments may be in English or Italian when matching nearby code, but keep them short and useful.

## Real-Time Audio Rules

Code in `processBlock` and DSP paths must stay real-time safe:

- Do not allocate memory, lock mutexes, perform file/network I/O, or log from the audio callback.
- Use atomics or lock-free handoff patterns for audio-to-UI state.
- Prepare buffers and DSP state in `prepareToPlay`.
- Keep parameter reads cheap and deterministic.
- Be careful with latency changes; update host latency consistently when phase or quality modes change.

## JUCE/APVTS Notes

- Parameters are defined through `AudioProcessorValueTreeState`; update the parameter layout and all dependent UI/DSP code together.
- Keep parameter IDs stable to avoid breaking saved sessions.
- Use `getRawParameterValue(...)` carefully and handle missing parameters defensively where appropriate.
- UI code should interact with parameters through attachments or APVTS-aware helpers where possible.

## CMake Notes

- The root `CMakeLists.txt` sets the project and C++ standard, adds `JUCE`, `libs/ui`, and `plugins`.
- `plugins/CMakeLists.txt` should add each plugin subdirectory.
- `libs/ui/CMakeLists.txt` owns the `ai_ui` target and public include directory.
- `plugins/AnalogEQ/CMakeLists.txt` owns plugin metadata, source lists, include paths, compile definitions, and JUCE module links.

## Testing and Verification

At minimum, verify CMake configuration and the affected plugin target:

```bash
cmake -S . -B build
cmake --build build --target AnalogEQ_VST3
```

For DSP changes, also check behavior at different sample rates and block sizes when possible. For UI changes, launch the standalone target or host the plugin and inspect resizing, parameter automation, and repaint performance.

## Repository Hygiene

- Do not commit build artifacts, plugin binaries, generated CMake files, or local caches.
- Keep edits scoped to the requested plugin/library.
- Do not rewrite JUCE submodule contents.
- Do not rename plugin IDs, manufacturer codes, bundle IDs, or parameter IDs unless the task explicitly requires a compatibility break.

