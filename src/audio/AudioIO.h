#pragma once

#include <string>
#include <vector>
#include <memory>

class AudioIO {
public:
    AudioIO();
    ~AudioIO();
    
    // Load audio file
    bool LoadFile(const std::string& path,
                  std::vector<std::vector<float>>& channels,
                  int& sampleRate,
                  int& numChannels,
                  size_t& numSamples);
    
    // Save audio file
    bool SaveFile(const std::string& path,
                  const std::vector<std::vector<float>>& channels,
                  int sampleRate,
                  int bitDepth = 24);
    
private:
    // Helper to interleave channels for libsndfile
    std::vector<float> InterleaveChannels(const std::vector<std::vector<float>>& channels);
    
    // Helper to deinterleave channels from libsndfile
    void DeinterleaveChannels(const std::vector<float>& interleaved,
                             std::vector<std::vector<float>>& channels,
                             int numChannels,
                             size_t numSamples);
};