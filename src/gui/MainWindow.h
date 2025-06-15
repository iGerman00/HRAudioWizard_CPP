#pragma once

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>

class AudioProcessor;
class FileDialog;

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    void Draw();

private:
    // UI State
    std::vector<std::string> inputFiles;
    std::string outputDirectory;
    bool showFileDialog = false;
    bool processing = false;
    std::atomic<float> progress{0.0f};
    std::string statusMessage = "Ready";
    std::string currentProcessingFile;
    
    // Processing settings
    bool enableHFC = true;
    bool compressedMode = false;
    int lowpassFreq = 16000;
    int sampleRateMultiplier = 2;  // 2x, 3x, 4x, etc.
    
    // Audio processor
    std::unique_ptr<AudioProcessor> audioProcessor;
    std::unique_ptr<std::thread> processingThread;
    
    // File dialog
    std::unique_ptr<FileDialog> fileDialog;
    
    // UI Methods
    void DrawMenuBar();
    void DrawFileSection();
    void DrawSettingsSection();
    void DrawProcessingSection();
    void DrawStatusBar();
    void DrawDropTarget();
    
    // File handling
    void AddFile(const std::string& path);
    void AddFiles();
    void ClearFiles();
    void ProcessFiles();
    void OnProcessingComplete();
    
    // Helpers
    std::string GetFileNameFromPath(const std::string& path) const;
    bool IsAudioFile(const std::string& path) const;
};