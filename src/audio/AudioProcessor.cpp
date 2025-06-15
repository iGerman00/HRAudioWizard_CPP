#include "AudioProcessor.h"
#include "AudioIO.h"
#include "HFCompensation.h"
#include "Resampler.h"
#include <iostream>
#include <algorithm>
#include <cmath>

AudioProcessor::AudioProcessor() {
}

AudioProcessor::~AudioProcessor() {
}

bool AudioProcessor::ProcessFile(const std::string& inputPath,
                                const std::string& outputPath,
                                bool enableHFC,
                                int lowpassFreq,
                                bool compressedMode,
                                int sampleRateMultiplier,
                                ProgressCallback progressCallback) {
    // Load audio file
    AudioData audio;
    if (!LoadAudioFile(inputPath, audio)) {
        std::cerr << "Failed to load audio file: " << inputPath << std::endl;
        return false;
    }
    
    std::cout << "Loaded audio: " << audio.numChannels << " channels, "
              << audio.numSamples << " samples, " << audio.sampleRate << " Hz" << std::endl;
    
    // For HF compensation, we upsample based on the multiplier
    if (enableHFC && sampleRateMultiplier > 1) {
        const int targetSampleRate = audio.sampleRate * sampleRateMultiplier;
        std::cout << "Upsampling from " << audio.sampleRate << " Hz to " << targetSampleRate << " Hz ("
                  << sampleRateMultiplier << "x)" << std::endl;
        
        // Upsample audio
        audio.channels = Resampler::ResampleMultiChannel(audio.channels, audio.sampleRate, targetSampleRate);
        audio.sampleRate = targetSampleRate;
        audio.numSamples = audio.channels[0].size();
        
        std::cout << "After upsampling: " << audio.numSamples << " samples at " << audio.sampleRate << " Hz" << std::endl;
    }
    
    // Process audio
    if (enableHFC) {
        ApplyHFC(audio, lowpassFreq, compressedMode, progressCallback);
    }
    
    // Verify we still have data
    if (audio.channels.empty() || audio.channels[0].empty()) {
        std::cerr << "Error: Audio data is empty after processing!" << std::endl;
        return false;
    }
    
    std::cout << "Processed audio: " << audio.channels.size() << " channels, " 
              << audio.channels[0].size() << " samples" << std::endl;
    
    // Save processed audio
    if (!SaveAudioFile(outputPath, audio)) {
        std::cerr << "Failed to save audio file: " << outputPath << std::endl;
        return false;
    }
    
    return true;
}

bool AudioProcessor::LoadAudioFile(const std::string& path, AudioData& audio) {
    AudioIO audioIO;
    return audioIO.LoadFile(path, audio.channels, audio.sampleRate, audio.numChannels, audio.numSamples);
}

bool AudioProcessor::SaveAudioFile(const std::string& path, const AudioData& audio) {
    AudioIO audioIO;
    return audioIO.SaveFile(path, audio.channels, audio.sampleRate);
}

void AudioProcessor::ApplyHFC(AudioData& audio, int lowpassFreq, bool compressedMode, 
                             ProgressCallback progressCallback) {
    if (audio.numChannels != 2) {
        std::cerr << "HFC requires stereo input" << std::endl;
        return;
    }
    
    // Convert to mid/side
    std::vector<float> mid, side;
    StereoToMidSide(audio.channels[0], audio.channels[1], mid, side);
    
    std::cout << "Before HFC - Mid size: " << mid.size() << ", Side size: " << side.size() << std::endl;
    
    // Apply HFC processing
    HFCompensation hfc;
    hfc.Process(mid, side, audio.sampleRate, lowpassFreq, compressedMode, progressCallback);
    
    std::cout << "After HFC - Mid size: " << mid.size() << ", Side size: " << side.size() << std::endl;
    
    // Convert back to stereo
    MidSideToStereo(mid, side, audio.channels[0], audio.channels[1]);
    
    // Update audio data size
    audio.numSamples = audio.channels[0].size();
    
    std::cout << "After conversion - Left size: " << audio.channels[0].size() 
              << ", Right size: " << audio.channels[1].size() << std::endl;
}

void AudioProcessor::StereoToMidSide(const std::vector<float>& left, 
                                    const std::vector<float>& right,
                                    std::vector<float>& mid,
                                    std::vector<float>& side) {
    size_t numSamples = left.size();
    mid.resize(numSamples);
    side.resize(numSamples);
    
    for (size_t i = 0; i < numSamples; ++i) {
        mid[i] = (left[i] + right[i]) * 0.5f;
        side[i] = (left[i] - right[i]) * 0.5f;
    }
}

void AudioProcessor::MidSideToStereo(const std::vector<float>& mid,
                                    const std::vector<float>& side,
                                    std::vector<float>& left,
                                    std::vector<float>& right) {
    size_t numSamples = mid.size();
    left.resize(numSamples);
    right.resize(numSamples);
    
    for (size_t i = 0; i < numSamples; ++i) {
        left[i] = mid[i] + side[i];
        right[i] = mid[i] - side[i];
    }
}