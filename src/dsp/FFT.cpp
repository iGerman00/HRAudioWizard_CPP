#include "FFT.h"
#include <kiss_fft.h>
#include <cstring>
#include <algorithm>

FFT::FFT(int size) : fftSize(size) {
    // Allocate KissFFT configurations
    fwdCfg = kiss_fft_alloc(fftSize, 0, nullptr, nullptr);
    invCfg = kiss_fft_alloc(fftSize, 1, nullptr, nullptr);
    
    // Pre-allocate buffers
    complexBuffer.resize(fftSize);
    realBuffer.resize(fftSize);
}

FFT::~FFT() {
    // Free KissFFT configurations
    if (fwdCfg) {
        kiss_fft_free(fwdCfg);
    }
    if (invCfg) {
        kiss_fft_free(invCfg);
    }
}

std::vector<std::complex<float>> FFT::Forward(const std::vector<float>& input) {
    // Prepare input buffer
    for (int i = 0; i < fftSize; ++i) {
        if (i < static_cast<int>(input.size())) {
            complexBuffer[i] = std::complex<float>(input[i], 0.0f);
        } else {
            complexBuffer[i] = std::complex<float>(0.0f, 0.0f);
        }
    }
    
    // Perform FFT
    std::vector<std::complex<float>> output(fftSize);
    kiss_fft(fwdCfg, 
             reinterpret_cast<const kiss_fft_cpx*>(complexBuffer.data()),
             reinterpret_cast<kiss_fft_cpx*>(output.data()));
    
    // Return only positive frequencies (including DC and Nyquist)
    output.resize(fftSize / 2 + 1);
    return output;
}

std::vector<float> FFT::Inverse(const std::vector<std::complex<float>>& input) {
    // Prepare full spectrum (mirror negative frequencies)
    for (int i = 0; i < fftSize / 2 + 1; ++i) {
        if (i < static_cast<int>(input.size())) {
            complexBuffer[i] = input[i];
        } else {
            complexBuffer[i] = std::complex<float>(0.0f, 0.0f);
        }
    }
    
    // Mirror for negative frequencies
    for (int i = 1; i < fftSize / 2; ++i) {
        complexBuffer[fftSize - i] = std::conj(complexBuffer[i]);
    }
    
    // Perform inverse FFT
    std::vector<std::complex<float>> tempOutput(fftSize);
    kiss_fft(invCfg,
             reinterpret_cast<const kiss_fft_cpx*>(complexBuffer.data()),
             reinterpret_cast<kiss_fft_cpx*>(tempOutput.data()));
    
    // Extract real part and normalize
    std::vector<float> output(fftSize);
    float scale = 1.0f / fftSize;
    for (int i = 0; i < fftSize; ++i) {
        output[i] = tempOutput[i].real() * scale;
    }
    
    return output;
}