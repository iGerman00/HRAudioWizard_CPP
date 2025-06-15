#pragma once

#include <vector>
#include <complex>
#include <memory>

class FFT;

class STFT {
public:
    STFT(int fftSize, int hopSize);
    ~STFT();
    
    // Forward STFT - returns complex spectrogram
    std::vector<std::vector<std::complex<float>>> Forward(const std::vector<float>& signal);
    
    // Inverse STFT - returns time-domain signal
    std::vector<float> Inverse(const std::vector<std::vector<std::complex<float>>>& spectrogram);
    
private:
    int fftSize;
    int hopSize;
    std::unique_ptr<FFT> fft;
    
    // Window function (Hann window)
    std::vector<float> window;
    
    // Helper functions
    void CreateWindow();
    std::vector<float> ApplyWindow(const std::vector<float>& frame);
};