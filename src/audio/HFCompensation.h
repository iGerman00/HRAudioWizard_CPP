#pragma once

#include <vector>
#include <complex>
#include <functional>

class HFCompensation {
public:
    using ProgressCallback = std::function<void(float)>;
    
    HFCompensation();
    ~HFCompensation();
    
    // Main HFC processing function
    void Process(std::vector<float>& mid,
                 std::vector<float>& side,
                 int sampleRate,
                 int lowpassFreq,
                 bool compressedMode,
                 ProgressCallback progressCallback = nullptr);
    
private:
    // STFT parameters
    static constexpr int FFTSIZE = 4096;
    static constexpr int HOPSIZE = 2048;
    
    // Overtone structure (from Python)
    struct Overtone {
        int width = 2;
        float amplitude = 0;
        int baseFreq = 0;
        std::vector<float> slope;
        int loop = 0;
        std::vector<float> power;
    };
    
    // Core processing functions
    void ProcessChannel(std::vector<std::vector<std::complex<float>>>& stftData,
                       int lowpassIdx,
                       bool isHarmonic);
    
    // Peak detection and harmonic removal
    std::vector<int> FindPeaks(const std::vector<float>& magnitude, int minDistance = 4);
    std::vector<int> RemoveHarmonics(const std::vector<int>& peaks);
    
    // Overtone synthesis
    void ProcessPeaks(const std::vector<int>& peaks,
                     const std::vector<float>& magnitude,
                     std::vector<float>& rebuild);
    
    // Spectral smoothing
    std::vector<float> FlattenSpectrum(const std::vector<float>& signal, int windowSize = 6);
    void TemporalSmoothing(std::vector<std::vector<float>>& spectrogram, int filterSize = 5);
    
    // Phase reconstruction (Griffin-Lim)
    std::vector<std::vector<std::complex<float>>> GriffinLim(
        const std::vector<std::vector<float>>& magnitude,
        int nIter = 2);
    
    // Spectrum connection
    std::vector<float> ConnectSpectraSmooth(const std::vector<float>& spectrum1,
                                           const std::vector<float>& spectrum2,
                                           int overlapSize = 16);
};