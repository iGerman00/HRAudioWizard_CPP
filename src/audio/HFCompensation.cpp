#include "HFCompensation.h"
#include "../dsp/STFT.h"
#include "../dsp/FFT.h"
#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <random>

HFCompensation::HFCompensation() {
}

HFCompensation::~HFCompensation() {
}

void HFCompensation::Process(std::vector<float>& mid,
                            std::vector<float>& side,
                            int sampleRate,
                            int lowpassFreq,
                            bool compressedMode,
                            ProgressCallback progressCallback) {
    // Calculate lowpass frequency index
    int lowpassIdx = static_cast<int>((FFTSIZE / 2 + 1) * (lowpassFreq / (sampleRate / 2.0f)));
    lowpassIdx = std::max(0, std::min(lowpassIdx, FFTSIZE / 2));
    
    std::cout << "Processing with lowpass at " << lowpassFreq << " Hz (bin " << lowpassIdx << ")" << std::endl;
    
    // Perform STFT
    STFT stft(FFTSIZE, HOPSIZE);
    auto midStft = stft.Forward(mid);
    auto sideStft = stft.Forward(side);
    
    int numFrames = midStft.size();
    
    // Process each frame
    for (int frame = 0; frame < numFrames; ++frame) {
        if (progressCallback) {
            progressCallback(static_cast<float>(frame) / numFrames);
        }
        
        // Get magnitude and phase
        std::vector<float> midMag(FFTSIZE / 2 + 1);
        std::vector<float> sideMag(FFTSIZE / 2 + 1);
        
        for (int i = 0; i < FFTSIZE / 2 + 1; ++i) {
            midMag[i] = std::abs(midStft[frame][i]);
            sideMag[i] = std::abs(sideStft[frame][i]);
        }
        
        // Save original low frequency content
        std::vector<std::complex<float>> midLowFreq(lowpassIdx);
        std::vector<std::complex<float>> sideLowFreq(lowpassIdx);
        for (int i = 0; i < lowpassIdx; ++i) {
            midLowFreq[i] = midStft[frame][i];
            sideLowFreq[i] = sideStft[frame][i];
        }
        
        // Detect peaks in the lower frequencies
        std::vector<int> midPeaks = FindPeaks(midMag);
        std::vector<int> sidePeaks = FindPeaks(sideMag);
        
        // Remove harmonics
        midPeaks = RemoveHarmonics(midPeaks);
        sidePeaks = RemoveHarmonics(sidePeaks);
        
        // Filter peaks to only include those below the lowpass frequency
        // Use more of the available range for better harmonic synthesis
        auto filterPeaks = [lowpassIdx](std::vector<int>& peaks) {
            peaks.erase(std::remove_if(peaks.begin(), peaks.end(),
                       [lowpassIdx](int p) { return p > lowpassIdx; }),
                       peaks.end());
        };
        
        filterPeaks(midPeaks);
        filterPeaks(sidePeaks);
        
        // Reconstruct high frequencies
        std::vector<float> midRebuild(FFTSIZE / 2 + 1, 0);
        std::vector<float> sideRebuild(FFTSIZE / 2 + 1, 0);
        
        ProcessPeaks(midPeaks, midMag, midRebuild);
        ProcessPeaks(sidePeaks, sideMag, sideRebuild);
        
        // Apply spectral smoothing
        midRebuild = FlattenSpectrum(midRebuild, 3);
        sideRebuild = FlattenSpectrum(sideRebuild, 5);
        
        // Apply random variation for naturalness
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.15125f, 1.0f);
        
        // Restore original low frequencies and add reconstructed high frequencies
        for (int i = 0; i < lowpassIdx; ++i) {
            midStft[frame][i] = midLowFreq[i];
            sideStft[frame][i] = sideLowFreq[i];
        }
        
        // Update the high frequency content
        for (int i = lowpassIdx; i < FFTSIZE / 2 + 1; ++i) {
            float fadeOut = std::pow(1.0f - static_cast<float>(i - lowpassIdx) / (FFTSIZE / 2 + 1 - lowpassIdx), 3);
            // Create complex numbers with magnitude and phase
            float midPhase = std::arg(midStft[frame][i]);
            float sidePhase = std::arg(sideStft[frame][i]);
            midStft[frame][i] = std::polar(midRebuild[i] * dist(gen) * fadeOut, midPhase);
            sideStft[frame][i] = std::polar(sideRebuild[i] * dist(gen) * fadeOut, sidePhase);
        }
    }
    
    // Inverse STFT
    mid = stft.Inverse(midStft);
    side = stft.Inverse(sideStft);
    
    // Ensure output vectors have correct size
    if (mid.empty() || side.empty()) {
        std::cerr << "Warning: HFC produced empty output!" << std::endl;
    }
    
    if (progressCallback) {
        progressCallback(1.0f);
    }
}

std::vector<int> HFCompensation::FindPeaks(const std::vector<float>& magnitude, int minDistance) {
    std::vector<int> peaks;
    
    for (size_t i = 1; i < magnitude.size() - 1; ++i) {
        // Check if it's a local maximum
        if (magnitude[i] > magnitude[i-1] && magnitude[i] > magnitude[i+1]) {
            // Check minimum distance from other peaks
            bool tooClose = false;
            for (int peak : peaks) {
                if (std::abs(static_cast<int>(i) - peak) < minDistance) {
                    tooClose = true;
                    break;
                }
            }
            
            if (!tooClose) {
                peaks.push_back(i);
            }
        }
    }
    
    return peaks;
}

std::vector<int> HFCompensation::RemoveHarmonics(const std::vector<int>& peaks) {
    std::vector<int> filtered;
    
    for (int peak : peaks) {
        bool isHarmonic = false;
        
        // Check if this peak is a harmonic of any lower frequency
        for (int fundamental : filtered) {
            int maxHarmonic = FFTSIZE / (2 * fundamental);
            for (int k = 2; k <= maxHarmonic; ++k) {
                if (std::abs(peak - fundamental * k) < 6) {
                    isHarmonic = true;
                    break;
                }
            }
            if (isHarmonic) break;
        }
        
        if (!isHarmonic) {
            filtered.push_back(peak);
        }
    }
    
    return filtered;
}

void HFCompensation::ProcessPeaks(const std::vector<int>& peaks,
                                 const std::vector<float>& magnitude,
                                 std::vector<float>& rebuild) {
    for (int peak : peaks) {
        Overtone ot;
        ot.baseFreq = peak;  // Use the actual peak frequency
        // Calculate how many harmonics can fit in the available spectrum
        ot.loop = std::min(12, (FFTSIZE / 2 - ot.baseFreq) / ot.baseFreq);
        
        // Extract harmonic amplitudes
        std::vector<float> harmonics;
        for (int l = 1; l < ot.loop && ot.baseFreq * l < static_cast<int>(magnitude.size()); ++l) {
            harmonics.push_back(magnitude[ot.baseFreq * l]);
        }
        
        if (harmonics.empty() || harmonics[0] == 0) continue;
        
        // Calculate slope using Gaussian weighting
        int gaussSize = harmonics.size();
        std::vector<float> gaussian(gaussSize);
        float sigma = gaussSize / 1.3f;
        for (int i = 0; i < gaussSize; ++i) {
            gaussian[i] = std::exp(-(i - gaussSize/2.0f) * (i - gaussSize/2.0f) / (2 * sigma * sigma));
        }
        
        // Normalize harmonics and apply Gaussian
        ot.slope.resize(ot.loop * 12);  // Extended for future overtones
        for (size_t i = 0; i < harmonics.size() && i < ot.slope.size(); ++i) {
            ot.slope[i] = (harmonics[i] / 12.0f) * gaussian[i % gaussian.size()];
        }
        
        // Determine width
        ot.width = 2;
        for (int k = 2; k <= 3; ++k) {
            if (peak - k/2 >= 0 && peak + k/2 < static_cast<int>(magnitude.size())) {
                if (std::abs(magnitude[peak - k/2] - magnitude[peak + k/2]) < 4) {
                    ot.width = k;
                    break;
                }
            }
        }
        
        // Extract power
        int startPower = std::max(0, peak - ot.width / 2);
        int endPower = std::min(static_cast<int>(magnitude.size()), peak + ot.width / 2);
        ot.power.resize(endPower - startPower);
        for (int i = startPower; i < endPower; ++i) {
            ot.power[i - startPower] = magnitude[i];
        }
        
        // Synthesize overtones - start from k=2 to synthesize above the fundamental
        for (int k = 2; k <= ot.loop + 1; ++k) {
            int start = ot.baseFreq * k - ot.width / 2;
            int end = ot.baseFreq * k + ot.width / 2;
            
            if (start < 0 || end > static_cast<int>(rebuild.size())) continue;
            
            if (k - 1 < static_cast<int>(ot.slope.size())) {
                // Apply decreasing amplitude for higher harmonics
                float harmonicAmp = std::abs(ot.slope[k - 1]) * std::pow(0.7f, k - 2);
                for (int i = start; i < end && i - start < static_cast<int>(ot.power.size()); ++i) {
                    rebuild[i] += ot.power[i - start] * harmonicAmp;
                }
            }
        }
    }
}

std::vector<float> HFCompensation::FlattenSpectrum(const std::vector<float>& signal, int windowSize) {
    std::vector<float> smoothed(signal.size());
    int halfWindow = windowSize / 2;
    
    for (size_t i = 0; i < signal.size(); ++i) {
        float sum = 0;
        int count = 0;
        
        for (int j = -halfWindow; j <= halfWindow; ++j) {
            int idx = i + j;
            if (idx >= 0 && idx < static_cast<int>(signal.size())) {
                sum += signal[idx];
                count++;
            }
        }
        
        smoothed[i] = count > 0 ? sum / count : signal[i];
    }
    
    return smoothed;
}