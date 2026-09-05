cmake_minimum_required(VERSION 3.20)

project(neta
    VERSION 0.1.0
    DESCRIPTION "NETA - retro cyberpunk narrative/simulation game (technical foundation)"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

# Version string available to C++ sources.
add_compile_definitions(NETA_VERSION_STRING="0.1.0")

# --- SDL2 (window, rendering, input, save-path helper) ---
# Prebuilt MinGW binaries, downloaded once per build dir and hash-pinned.
# Rationale: building SDL from source breaks when the project path contains
# spaces (windres) and costs minutes per clean configure; the official
# libsdl-org binaries are the same code, verified by SHA-256 below.
set(SDL2_VERSION "2.32.10")
set(SDL2_PACKAGE_URL
    "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-devel-2.32.10-mingw.tar.gz")
set(SDL2_PACKAGE_SHA256 "83A5D74012311EDC3C0D40EA6FAECBE57AD692AA033FA5DC273CC937E3938FF2")
set(SDL2_ROOT_DIR "${CMAKE_BINARY_DIR}/_sdl2/SDL2-${SDL2_VERSION}/x86_64-w64-mingw32")
if(NOT EXISTS "${SDL2_ROOT_DIR}/include/SDL2/SDL.h")
    message(STATUS "Downloading SDL2 ${SDL2_VERSION} prebuilt binaries...")
    file(DOWNLOAD "${SDL2_PACKAGE_URL}" "${CMAKE_BINARY_DIR}/_sdl2/sdl2-mingw.tar.gz"
        EXPECTED_HASH SHA256=${SDL2_PACKAGE_SHA256}
        SHOW_PROGRESS)
    file(ARCHIVE_EXTRACT INPUT "${CMAKE_BINARY_DIR}/_sdl2/sdl2-mingw.tar.gz"
        DESTINATION "${CMAKE_BINARY_DIR}/_sdl2")
endif()
if(NOT EXISTS "${SDL2_ROOT_DIR}/include/SDL2/SDL.h")
    message(FATAL_ERROR "SDL2 headers missing after extract at ${SDL2_ROOT_DIR}")
endif()

add_library(SDL2::SDL2 SHARED IMPORTED GLOBAL)
set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION "${SDL2_ROOT_DIR}/bin/SDL2.dll"
    IMPORTED_IMPLIB "${SDL2_ROOT_DIR}/lib/libSDL2.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_ROOT_DIR}/include/SDL2")
add_library(SDL2::SDL2main STATIC IMPORTED GLOBAL)
set_target_properties(SDL2::SDL2main PROPERTIES
    IMPORTED_LOCATION "${SDL2_ROOT_DIR}/lib/libSDL2main.a"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_ROOT_DIR}/include/SDL2")

# --- Game library (everything except the entry point) ---
set(NETA_SOURCES
    src/core/Game.cpp
    src/core/Config.cpp
    src/core/Log.cpp
    src/core/Random.cpp
    src/simulation/Simulation.cpp
    src/world/World.cpp
    src/entities/Player.cpp
    src/entities/Npc.cpp
    src/entities/Faction.cpp
    src/ai/AgentMind.cpp
    src/narrative/NarrativeState.cpp
    src/rendering/Renderer.cpp
    src/rendering/PixelFont.cpp
    src/input/InputManager.cpp
    src/audio/AudioSystem.cpp
    src/ui/Ui.cpp
    src/save/SaveSystem.cpp
)

add_library(netalib STATIC ${NETA_SOURCES})
target_include_directories(netalib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(netalib PUBLIC SDL2::SDL2)

# --- Executable ---
add_executable(neta src/main/main.cpp)
# mingw32 must precede SDL2main for WinMain linkage with MinGW.
target_link_libraries(neta PRIVATE netalib mingw32 SDL2::SDL2main SDL2::SDL2)

if(MSVC)
    target_compile_options(netalib PRIVATE /W4 /permissive-)
    target_compile_options(neta PRIVATE /W4 /permissive-)
else()
    target_compile_options(netalib PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(neta PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Distributable binaries: fully static GCC runtime (libgcc, libstdc++,
# winpthread) so the exe runs on a clean Windows machine with only SDL2.dll
# beside it. System APIs and SDL2 still link via import libs (.dll.a), so
# they correctly remain DLL dependencies that every Windows machine has.
if(MINGW)
    target_link_options(neta PRIVATE -static)
endif()

# Keep the console window on Windows: logs are printed there during development.
# (A future Steam Release build may switch to a windowed subsystem.)

# Ship SDL2.dll next to the exe so Release builds run on a clean machine.
add_custom_command(TARGET neta POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SDL2_ROOT_DIR}/bin/SDL2.dll"
        $<TARGET_FILE_DIR:neta>
)

# --- Self-tests ---
option(NETA_BUILD_TESTS "Build determinism/self tests" ON)
if(NETA_BUILD_TESTS)
    enable_testing()
    add_executable(neta_tests tests/test_determinism.cpp)
    target_link_libraries(neta_tests PRIVATE netalib mingw32 SDL2::SDL2main SDL2::SDL2)
    if(MINGW)
        target_link_options(neta_tests PRIVATE -static)
    endif()
    if(NOT MSVC)
        target_compile_options(neta_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_custom_command(TARGET neta_tests POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL2_ROOT_DIR}/bin/SDL2.dll"
            $<TARGET_FILE_DIR:neta_tests>
    )
    add_test(NAME determinism COMMAND neta_tests)
    add_test(NAME headless_sim COMMAND neta --headless --ticks 50 --seed 482913)
endif()

install(TARGETS neta RUNTIME DESTINATION .)
