#!/bin/bash

echo "Building HRAudioWizard..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
make -j$(sysctl -n hw.ncpu)

if [ $? -eq 0 ]; then
    echo "Build successful!"
    echo "Executable is at: build/bin/HRAudioWizard"
else
    echo "Build failed!"
    exit 1
fi