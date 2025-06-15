#include "STFT.h"
#include "FFT.h"
#include <cmath>
#include <algorithm>

STFT::STFT(int fftSize, int hopSize) 
    : fftSize(fftSize), hopSize(hopSize) {
    fft = std::make_unique<FFT>(fftSize);
    CreateWindow();
}

STFT::~STFT() {
}

void STFT::CreateWindow() {
    window.resize(fftSize);
    // Hann window
    for (int i = 0; i < fftSize; ++i) {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fftSize - 1)));
    }
}

std::vector<float> STFT::ApplyWindow(const std::vector<float>& frame) {
    std::vector<float> windowed(frame.size());
    for (size_t i = 0; i < frame.size(); ++i) {
        windowed[i] = frame[i] * window[i];
    }
    return windowed;
}

std::vector<std::vector<std::complex<float>>> STFT::Forward(const std::vector<float>& signal) {
    int numFrames = (signal.size() - fftSize) / hopSize + 1;
    std::vector<std::vector<std::complex<float>>> spectrogram(numFrames);
    
    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
        int startIdx = frameIdx * hopSize;
        
        // Extract frame
        std::vector<float> frame(fftSize);
        for (int i = 0; i < fftSize; ++i) {
            if (startIdx + i < static_cast<int>(signal.size())) {
                frame[i] = signal[startIdx + i];
            } else {
                frame[i] = 0.0f; // Zero padding
            }
        }
        
        // Apply window
        frame = ApplyWindow(frame);
        
        // Perform FFT
        spectrogram[frameIdx] = fft->Forward(frame);
    }
    
    return spectrogram;
}

std::vector<float> STFT::Inverse(const std::vector<std::vector<std::complex<float>>>& spectrogram) {
    if (spectrogram.empty()) {
        return {};
    }
    
    int numFrames = spectrogram.size();
    int outputSize = (numFrames - 1) * hopSize + fftSize;
    std::vector<float> output(outputSize, 0.0f);
    std::vector<float> windowSum(outputSize, 0.0f);
    
    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx) {
        // Perform inverse FFT
        std::vector<float> frame = fft->Inverse(spectrogram[frameIdx]);
        
        // Apply window and overlap-add
        int startIdx = frameIdx * hopSize;
        for (int i = 0; i < fftSize; ++i) {
            if (startIdx + i < outputSize) {
                output[startIdx + i] += frame[i] * window[i];
                windowSum[startIdx + i] += window[i] * window[i];
            }
        }
    }
    
    // Normalize by window sum to maintain amplitude
    for (int i = 0; i < outputSize; ++i) {
        if (windowSum[i] > 0.0f) {
            output[i] /= windowSum[i];
        }
    }
    
    return output;
}