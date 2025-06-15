#include "MainWindow.h"
#include "FileDialog.h"
#include "../audio/AudioProcessor.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;

MainWindow::MainWindow() {
    audioProcessor = std::make_unique<AudioProcessor>();
    fileDialog = std::make_unique<FileDialog>();
}

MainWindow::~MainWindow() {
    if (processingThread && processingThread->joinable()) {
        processingThread->join();
    }
}

void MainWindow::Draw() {
    // Create main window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
                                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    
    ImGui::Begin("HRAudioWizard", nullptr, window_flags);
    
    DrawMenuBar();
    
    ImGui::BeginChild("MainContent", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 2));
    
    DrawFileSection();
    ImGui::Separator();
    DrawSettingsSection();
    ImGui::Separator();
    DrawProcessingSection();
    ImGui::EndChild();
    
    DrawStatusBar();
    
    ImGui::End();
    
    // Show file dialog if needed
    if (showFileDialog) {
        fileDialog->Show(&showFileDialog, [this](const std::string& path) {
            AddFile(path);
        });
    }
}

void MainWindow::DrawMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Add Files...", "Cmd+O")) {
                showFileDialog = true;
            }
            if (ImGui::MenuItem("Clear All", nullptr, false, !inputFiles.empty())) {
                ClearFiles();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Cmd+Q")) {
                exit(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                ImGui::OpenPopup("About");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    // About popup
    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("HRAudioWizard C++");
        ImGui::Text("High-quality audio enhancement tool");
        ImGui::Text("Based on the Python HRAudioWizard (MIT License)");
        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void MainWindow::DrawDropTarget() {
    // Simple drop zone indicator
    // Note: Actual drag & drop would require platform-specific implementation
}

void MainWindow::DrawFileSection() {
    ImGui::Text("Input Files");
    
    // File list
    ImGui::BeginChild("FileList", ImVec2(-1, 150), true);
    
    for (size_t i = 0; i < inputFiles.size(); i++) {
        std::string label = std::to_string(i + 1) + ". " + GetFileNameFromPath(inputFiles[i]);
        
        // Add a remove button for each file
        std::string buttonId = "X##" + std::to_string(i);
        if (ImGui::SmallButton(buttonId.c_str())) {
            inputFiles.erase(inputFiles.begin() + i);
            statusMessage = "Removed file";
            break;
        }
        ImGui::SameLine();
        
        ImGui::Text("%s", label.c_str());
    }
    
    // Show drag & drop hint if empty
    if (inputFiles.empty()) {
        ImGui::TextDisabled("Click 'Add Files' to select audio files");
    }
    
    ImGui::EndChild();
    
    // Buttons
    if (ImGui::Button("Add Files", ImVec2(100, 0))) {
        showFileDialog = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear All", ImVec2(100, 0))) {
        ClearFiles();
    }
    ImGui::SameLine();
    ImGui::Text("%zu files selected", inputFiles.size());
}

void MainWindow::DrawSettingsSection() {
    ImGui::Text("Processing Settings");
    
    ImGui::Checkbox("Enable High Frequency Compensation", &enableHFC);
    
    if (enableHFC) {
        ImGui::Indent();
        ImGui::SliderInt("Lowpass Frequency (Hz)", &lowpassFreq, 6000, 192000);
        
        // Sample rate multiplier as a slider
        ImGui::SliderInt("Sample Rate Multiplier", &sampleRateMultiplier, 1, 16);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Multiplies the input sample rate for the output file\nHigher values allow more high frequency synthesis");
        }
        
        // Show the resulting sample rate
        ImGui::TextDisabled("(Output will be input sample rate × %d)", sampleRateMultiplier);
        
        ImGui::Checkbox("Compressed Source Mode", &compressedMode);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Optimized settings for heavily compressed audio sources");
        }
        ImGui::Unindent();
    }
}

void MainWindow::DrawProcessingSection() {
    ImGui::Text("Processing");
    
    // Show current file being processed
    if (processing && !currentProcessingFile.empty()) {
        ImGui::Text("Processing: %s", GetFileNameFromPath(currentProcessingFile).c_str());
    }
    
    // Progress bar
    ImGui::ProgressBar(progress.load(), ImVec2(-1, 0));
    
    // Process button
    bool canProcess = !inputFiles.empty() && !processing;
    if (!canProcess) {
        ImGui::BeginDisabled();
    }
    
    if (ImGui::Button("Process Files", ImVec2(150, 30))) {
        ProcessFiles();
    }
    
    if (!canProcess) {
        ImGui::EndDisabled();
    }
    
    if (processing) {
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            // TODO: Implement cancellation
            statusMessage = "Cancellation not yet implemented";
        }
    }
}

void MainWindow::DrawStatusBar() {
    ImGui::Separator();
    ImGui::Text("Status: %s", statusMessage.c_str());
}

void MainWindow::AddFile(const std::string& path) {
    if (!IsAudioFile(path)) {
        statusMessage = "Not a valid audio file: " + GetFileNameFromPath(path);
        return;
    }
    
    // Check if file already exists in the list
    if (std::find(inputFiles.begin(), inputFiles.end(), path) != inputFiles.end()) {
        statusMessage = "File already in list: " + GetFileNameFromPath(path);
        return;
    }
    
    inputFiles.push_back(path);
    statusMessage = "Added: " + GetFileNameFromPath(path);
}

void MainWindow::AddFiles() {
    showFileDialog = true;
}

void MainWindow::ClearFiles() {
    inputFiles.clear();
    statusMessage = "File list cleared";
}

void MainWindow::ProcessFiles() {
    if (inputFiles.empty() || processing) return;
    
    // Make sure previous thread is finished
    if (processingThread && processingThread->joinable()) {
        processingThread->join();
    }
    
    processing = true;
    progress = 0.0f;
    statusMessage = "Processing...";
    
    // Process files in a separate thread
    processingThread = std::make_unique<std::thread>([this]() {
        int totalFiles = inputFiles.size();
        int successCount = 0;
        
        for (int i = 0; i < totalFiles; i++) {
            // Update current file being processed
            currentProcessingFile = inputFiles[i];
            
            // Generate output path
            fs::path inputPath(inputFiles[i]);
            fs::path outputPath = inputPath.parent_path() / 
                (inputPath.stem().string() + "_enhanced" + inputPath.extension().string());
            
            statusMessage = "Processing: " + GetFileNameFromPath(inputFiles[i]);
            
            // Call the audio processor
            bool success = audioProcessor->ProcessFile(
                inputFiles[i],
                outputPath.string(),
                enableHFC,
                lowpassFreq,
                compressedMode,
                sampleRateMultiplier,
                [this, i, totalFiles](float fileProgress) {
                    // Update overall progress
                    float overallProgress = (i + fileProgress) / totalFiles;
                    progress = overallProgress;
                }
            );
            
            if (success) {
                successCount++;
                statusMessage = "Completed: " + GetFileNameFromPath(inputFiles[i]);
            } else {
                statusMessage = "Error processing: " + GetFileNameFromPath(inputFiles[i]);
            }
        }
        
        // Final status
        std::stringstream ss;
        ss << "Processing complete! " << successCount << "/" << totalFiles << " files processed successfully";
        statusMessage = ss.str();
        currentProcessingFile.clear();
        
        OnProcessingComplete();
    });
}

void MainWindow::OnProcessingComplete() {
    processing = false;
    progress = 1.0f;
}

std::string MainWindow::GetFileNameFromPath(const std::string& path) const {
    return fs::path(path).filename().string();
}

bool MainWindow::IsAudioFile(const std::string& path) const {
    static const std::vector<std::string> audioExtensions = {
        ".wav", ".flac", ".ogg", ".mp3", ".aiff", ".aif", ".m4a", ".opus"
    };
    
    std::string ext = fs::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return std::find(audioExtensions.begin(), audioExtensions.end(), ext) != audioExtensions.end();
}