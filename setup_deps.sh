#!/bin/bash

echo "Setting up dependencies for HRAudioWizard..."

# Create deps directory if it doesn't exist
mkdir -p deps

cd deps

# Download Dear ImGui
if [ ! -d "imgui" ]; then
    echo "Downloading Dear ImGui..."
    git clone https://github.com/ocornut/imgui.git
    cd imgui
    git checkout docking  # Use docking branch which is stable
    cd ..
fi

# Download GLFW
if [ ! -d "glfw" ]; then
    echo "Downloading GLFW..."
    git clone https://github.com/glfw/glfw.git
    cd glfw
    git checkout 3.3.9
    cd ..
fi

# Download KissFFT (complete version)
if [ ! -d "kissfft" ]; then
    echo "Downloading KissFFT..."
    git clone https://github.com/mborgerding/kissfft.git kissfft_temp
    mkdir -p kissfft
    # Copy only the necessary files
    cp kissfft_temp/kiss_fft.h kissfft/
    cp kissfft_temp/kiss_fft.c kissfft/
    cp kissfft_temp/_kiss_fft_guts.h kissfft/
    # Create a simple kiss_fft_log.h since it's not in the main repo (??? wtf claude XDDDDDD)
    cat > kissfft/kiss_fft_log.h << 'EOF'
#ifndef KISS_FFT_LOG_H
#define KISS_FFT_LOG_H

// Simple logging macros for KissFFT
#define KISS_FFT_LOG_MSG(...) ((void)0)
#define KISS_FFT_WARNING(...) ((void)0)
#define KISS_FFT_ERROR(...) ((void)0)
#define KISS_FFT_LOG_USE_C_STDIO 0

#endif // KISS_FFT_LOG_H
EOF
    rm -rf kissfft_temp
fi

cd ..

# Install libsndfile via Homebrew
echo "Checking for libsndfile..."
if ! brew list libsndfile &>/dev/null; then
    echo "Installing libsndfile..."
    brew install libsndfile
else
    echo "libsndfile is already installed"
fi

# Check for pkg-config
echo "Checking for pkg-config..."
if ! brew list pkg-config &>/dev/null; then
    echo "Installing pkg-config..."
    brew install pkg-config
else
    echo "pkg-config is already installed"
fi

echo "Dependencies setup complete!"