cmake_minimum_required(VERSION 3.28)
project(nbody VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    # Read every source file as UTF-8
    add_compile_options(/utf-8)
endif()

# CMake 4 refuses projects that ask for a minimum version below 3.5
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)

# Use an installed SFML 3 if there is one (Homebrew on macOS)
find_package(SFML 3 COMPONENTS Graphics Window System QUIET)
if(NOT SFML_FOUND)
    message(STATUS "SFML 3 not installed, building it from source")
    set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.1
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
    FetchContent_MakeAvailable(SFML)
endif()

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.91.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(imgui)

set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF)
FetchContent_Declare(imgui-sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml
    GIT_TAG v3.0
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(imgui-sfml)

# The physics, the JPL Horizons download and the calendar: everything with no
# graphics, so the tests can link it without SFML
add_library(emma_core STATIC
    Simulation.cpp
    Body.cpp
    QuadTree.cpp
    Calendar.cpp
    Horizons.cpp
    Http.cpp)
target_include_directories(emma_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
    target_link_libraries(emma_core PUBLIC winhttp)  # HTTPS on Windows, built into the OS
endif()

add_executable(nbody
    main.cpp
    Renderer.cpp
    Ui.cpp)
target_link_libraries(nbody PRIVATE emma_core SFML::Graphics ImGui-SFML::ImGui-SFML)

if(MSVC)
    # Release builds are windowed apps, with no console window behind EMMA
    target_link_options(nbody PRIVATE
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
        $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>)
endif()

# Copy the font next to the executable, so EMMA finds it from any working directory
add_custom_command(TARGET nbody POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:nbody>/fonts
    VERBATIM)

# Physics tests: ctest --test-dir build -C Release --output-on-failure
enable_testing()
add_executable(physics_tests tests/physics_tests.cpp)
target_link_libraries(physics_tests PRIVATE emma_core)
foreach(test calendar horizons_parse j2000_start earth_year conservation close_pass head_on_merge barnes_hut
             dominant_body trail_smoothness)
    add_test(NAME ${test} COMMAND physics_tests ${test})
endforeach()
