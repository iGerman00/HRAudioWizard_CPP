#include "AudioIO.h"
#include <sndfile.h>
#include <iostream>
#include <cstring>
#include <cmath>
#include <limits>

AudioIO::AudioIO() {
}

AudioIO::~AudioIO() {
}

bool AudioIO::LoadFile(const std::string& path,
                      std::vector<std::vector<float>>& channels,
                      int& sampleRate,
                      int& numChannels,
                      size_t& numSamples) {
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));
    
    SNDFILE* sndfile = sf_open(path.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        std::cerr << "Error opening file: " << sf_strerror(nullptr) << std::endl;
        return false;
    }
    
    sampleRate = sfinfo.samplerate;
    numChannels = sfinfo.channels;
    numSamples = sfinfo.frames;
    
    // Read all samples
    std::vector<float> interleaved(numSamples * numChannels);
    sf_count_t framesRead = sf_readf_float(sndfile, interleaved.data(), numSamples);
    
    if (framesRead != static_cast<sf_count_t>(numSamples)) {
        std::cerr << "Error reading file: expected " << numSamples << " frames, got " << framesRead << std::endl;
        sf_close(sndfile);
        return false;
    }
    
    // Deinterleave channels
    DeinterleaveChannels(interleaved, channels, numChannels, numSamples);
    
    sf_close(sndfile);
    return true;
}

bool AudioIO::SaveFile(const std::string& path,
                      const std::vector<std::vector<float>>& channels,
                      int sampleRate,
                      int bitDepth) {
    if (channels.empty() || channels[0].empty()) {
        std::cerr << "No audio data to save" << std::endl;
        return false;
    }
    
    std::cout << "SaveFile: channels.size() = " << channels.size() << std::endl;
    std::cout << "SaveFile: channels[0].size() = " << channels[0].size() << std::endl;
    if (channels.size() > 1) {
        std::cout << "SaveFile: channels[1].size() = " << channels[1].size() << std::endl;
    }
    
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));
    sfinfo.samplerate = sampleRate;
    sfinfo.channels = channels.size();
    sfinfo.frames = 0;  // For write mode, this should be 0
    
    // Store the actual frame count separately
    sf_count_t framesToWrite = channels[0].size();
    
    // Use float format for better compatibility with our float data
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
    
    // Validate the format
    if (!sf_format_check(&sfinfo)) {
        std::cerr << "Invalid format specification" << std::endl;
        return false;
    }
    
    std::cout << "SaveFile: Writing " << framesToWrite << " frames at " << sfinfo.samplerate << " Hz" << std::endl;
    std::cout << "SaveFile: Format = 0x" << std::hex << sfinfo.format << std::dec << std::endl;
    
    SNDFILE* sndfile = sf_open(path.c_str(), SFM_WRITE, &sfinfo);
    if (!sndfile) {
        std::cerr << "Error creating file: " << sf_strerror(nullptr) << std::endl;
        return false;
    }
    
    // Interleave channels and write
    std::vector<float> interleaved = InterleaveChannels(channels);
    std::cout << "SaveFile: Interleaved size = " << interleaved.size() << std::endl;
    
    // Check for NaN or infinite values and fix them
    int nanCount = 0;
    int infCount = 0;
    int clampCount = 0;
    for (size_t i = 0; i < interleaved.size(); ++i) {
        if (std::isnan(interleaved[i])) {
            interleaved[i] = 0.0f;
            nanCount++;
        } else if (std::isinf(interleaved[i])) {
            interleaved[i] = interleaved[i] > 0 ? 1.0f : -1.0f;
            infCount++;
        } else if (interleaved[i] > 1.0f) {
            interleaved[i] = 1.0f;
            clampCount++;
        } else if (interleaved[i] < -1.0f) {
            interleaved[i] = -1.0f;
            clampCount++;
        }
    }
    
    if (nanCount > 0 || infCount > 0 || clampCount > 0) {
        std::cout << "Warning: Fixed " << nanCount << " NaN values, " << infCount 
                  << " infinite values, and " << clampCount << " out-of-range values" << std::endl;
    }
    
    // Check a few samples
    std::cout << "First few samples: ";
    for (int i = 0; i < std::min(10, (int)interleaved.size()); ++i) {
        std::cout << interleaved[i] << " ";
    }
    std::cout << std::endl;
    
    // Clear any error state
    sf_error(sndfile);
    
    // Write the data
    sf_count_t framesWritten = sf_writef_float(sndfile, interleaved.data(), framesToWrite);
    
    // Get error immediately after write
    int error = sf_error(sndfile);
    if (error != SF_ERR_NO_ERROR) {
        std::cerr << "Write error: " << sf_strerror(sndfile) << std::endl;
    }
    
    std::cout << "SaveFile: Wrote " << framesWritten << " frames" << std::endl;
    
    if (framesWritten != framesToWrite) {
        std::cerr << "Error writing file: expected " << framesToWrite << " frames, wrote " << framesWritten << std::endl;
        sf_close(sndfile);
        return false;
    }
    
    sf_close(sndfile);
    return true;
}

std::vector<float> AudioIO::InterleaveChannels(const std::vector<std::vector<float>>& channels) {
    if (channels.empty() || channels[0].empty()) return {};
    
    size_t numChannels = channels.size();
    size_t numSamples = channels[0].size();
    std::vector<float> interleaved(numChannels * numSamples);
    
    for (size_t i = 0; i < numSamples; ++i) {
        for (size_t ch = 0; ch < numChannels; ++ch) {
            interleaved[i * numChannels + ch] = channels[ch][i];
        }
    }
    
    return interleaved;
}

void AudioIO::DeinterleaveChannels(const std::vector<float>& interleaved,
                                  std::vector<std::vector<float>>& channels,
                                  int numChannels,
                                  size_t numSamples) {
    channels.resize(numChannels);
    for (int ch = 0; ch < numChannels; ++ch) {
        channels[ch].resize(numSamples);
    }
    
    for (size_t i = 0; i < numSamples; ++i) {
        for (int ch = 0; ch < numChannels; ++ch) {
            channels[ch][i] = interleaved[i * numChannels + ch];
        }
    }
}