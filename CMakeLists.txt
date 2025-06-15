cmake_minimum_required(VERSION 3.16)
project(HRAudioWizard VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
endif()

# Find packages
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Find libsndfile with pkg-config
pkg_check_modules(SNDFILE REQUIRED sndfile)

# Platform-specific settings
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    set(EXTRA_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
endif()

# Add subdirectories for dependencies
add_subdirectory(deps)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/deps/imgui
    ${CMAKE_SOURCE_DIR}/deps/imgui/backends
    ${CMAKE_SOURCE_DIR}/deps/glfw/include
    ${CMAKE_SOURCE_DIR}/deps/kissfft
    ${SNDFILE_INCLUDE_DIRS}
)

# Link directories for libsndfile
link_directories(${SNDFILE_LIBRARY_DIRS})

# Source files
set(SOURCES
    src/main.cpp
    src/gui/MainWindow.cpp
    src/gui/FileDialog.cpp
    src/audio/AudioProcessor.cpp
    src/audio/HFCompensation.cpp
    src/audio/AudioIO.cpp
    src/audio/Resampler.cpp
    src/dsp/FFT.cpp
    src/dsp/STFT.cpp
)

set(HEADERS
    src/gui/MainWindow.h
    src/gui/FileDialog.h
    src/audio/AudioProcessor.h
    src/audio/HFCompensation.h
    src/audio/AudioIO.h
    src/audio/Resampler.h
    src/dsp/FFT.h
    src/dsp/STFT.h
)

# ImGui sources
set(IMGUI_SOURCES
    deps/imgui/imgui.cpp
    deps/imgui/imgui_demo.cpp
    deps/imgui/imgui_draw.cpp
    deps/imgui/imgui_tables.cpp
    deps/imgui/imgui_widgets.cpp
    deps/imgui/backends/imgui_impl_glfw.cpp
    deps/imgui/backends/imgui_impl_opengl3.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

# Add compile options for libsndfile
target_compile_options(${PROJECT_NAME} PRIVATE ${SNDFILE_CFLAGS_OTHER})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    glfw
    kissfft
    ${SNDFILE_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBS}
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)