#pragma once

#include <string>
#include <functional>
#include <vector>
#include <complex>

class AudioProcessor {
public:
    using ProgressCallback = std::function<void(float)>;
    
    AudioProcessor();
    ~AudioProcessor();
    
    // Main processing function
    bool ProcessFile(const std::string& inputPath,
                    const std::string& outputPath,
                    bool enableHFC,
                    int lowpassFreq,
                    bool compressedMode,
                    int sampleRateMultiplier = 2,
                    ProgressCallback progressCallback = nullptr);
    
private:
    // Audio data structure
    struct AudioData {
        std::vector<std::vector<float>> channels;  // [channel][sample]
        int sampleRate;
        int numChannels;
        size_t numSamples;
    };
    
    // Processing helpers
    bool LoadAudioFile(const std::string& path, AudioData& audio);
    bool SaveAudioFile(const std::string& path, const AudioData& audio);
    
    // HFC processing
    void ApplyHFC(AudioData& audio, int lowpassFreq, bool compressedMode, 
                  ProgressCallback progressCallback);
    
    // Convert stereo to mid/side
    void StereoToMidSide(const std::vector<float>& left, 
                        const std::vector<float>& right,
                        std::vector<float>& mid,
                        std::vector<float>& side);
    
    // Convert mid/side back to stereo
    void MidSideToStereo(const std::vector<float>& mid,
                        const std::vector<float>& side,
                        std::vector<float>& left,
                        std::vector<float>& right);
};