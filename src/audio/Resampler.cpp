#include "Resampler.h"
#include <cmath>

std::vector<float> Resampler::Resample(const std::vector<float>& input, 
                                       int inputSampleRate, 
                                       int outputSampleRate) {
    if (inputSampleRate == outputSampleRate) {
        return input;
    }
    
    double ratio = static_cast<double>(outputSampleRate) / inputSampleRate;
    size_t outputSize = static_cast<size_t>(input.size() * ratio);
    std::vector<float> output(outputSize);
    
    // Simple linear interpolation
    for (size_t i = 0; i < outputSize; ++i) {
        double srcIndex = i / ratio;
        size_t srcIdx = static_cast<size_t>(srcIndex);
        double fraction = srcIndex - srcIdx;
        
        if (srcIdx < input.size() - 1) {
            // Linear interpolation between two samples
            output[i] = input[srcIdx] * (1.0 - fraction) + input[srcIdx + 1] * fraction;
        } else if (srcIdx < input.size()) {
            // Last sample
            output[i] = input[srcIdx];
        } else {
            // Beyond input range (shouldn't happen)
            output[i] = 0.0f;
        }
    }
    
    return output;
}

std::vector<std::vector<float>> Resampler::ResampleMultiChannel(
    const std::vector<std::vector<float>>& channels,
    int inputSampleRate,
    int outputSampleRate) {
    
    std::vector<std::vector<float>> output;
    output.reserve(channels.size());
    
    for (const auto& channel : channels) {
        output.push_back(Resample(channel, inputSampleRate, outputSampleRate));
    }
    
    return output;
}