#!/bin/bash

# why does this file even exist lmfao claude

echo "Starting HRAudioWizard..."

# Check if the executable exists
if [ ! -f "build/bin/HRAudioWizard" ]; then
    echo "Error: HRAudioWizard executable not found!"
    echo "Please run ./build.sh first to build the application."
    exit 1
fi

# Run the application
./build/bin/HRAudioWizard