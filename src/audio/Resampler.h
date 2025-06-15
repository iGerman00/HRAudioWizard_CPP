#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <vector>

class Resampler {
public:
    // Simple linear interpolation resampler
    static std::vector<float> Resample(const std::vector<float>& input, 
                                      int inputSampleRate, 
                                      int outputSampleRate);
    
    // Resample multiple channels
    static std::vector<std::vector<float>> ResampleMultiChannel(
        const std::vector<std::vector<float>>& channels,
        int inputSampleRate,
        int outputSampleRate);
};

#endif // RESAMPLER_H