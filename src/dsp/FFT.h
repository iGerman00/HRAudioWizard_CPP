#pragma once

#include <vector>
#include <complex>
#include <memory>

// Forward declaration for KissFFT
struct kiss_fft_state;
typedef struct kiss_fft_state* kiss_fft_cfg;

class FFT {
public:
    FFT(int size);
    ~FFT();
    
    // Forward FFT - real to complex
    std::vector<std::complex<float>> Forward(const std::vector<float>& input);
    
    // Inverse FFT - complex to real
    std::vector<float> Inverse(const std::vector<std::complex<float>>& input);
    
private:
    int fftSize;
    kiss_fft_cfg fwdCfg;
    kiss_fft_cfg invCfg;
    
    // Pre-allocated buffers
    std::vector<std::complex<float>> complexBuffer;
    std::vector<float> realBuffer;
};