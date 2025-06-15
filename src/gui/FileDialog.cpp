#include "FileDialog.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

FileDialog::FileDialog() {
    // Start in the user's home directory
    currentPath = fs::path(getenv("HOME"));
    
    // Default audio file extensions
    extensionFilter = {".wav", ".flac", ".ogg", ".mp3", ".aiff", ".aif", ".m4a"};
}

FileDialog::~FileDialog() {
}

void FileDialog::SetExtensionFilter(const std::vector<std::string>& extensions) {
    extensionFilter = extensions;
}

bool FileDialog::HasValidExtension(const fs::path& path) const {
    if (extensionFilter.empty()) return true;
    
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    for (const auto& filter : extensionFilter) {
        std::string filterLower = filter;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);
        if (ext == filterLower) return true;
    }
    return false;
}

void FileDialog::NavigateTo(const fs::path& path) {
    if (fs::exists(path) && fs::is_directory(path)) {
        currentPath = path;
    }
}

void FileDialog::Show(bool* p_open, std::function<void(const std::string&)> onFileSelected) {
    if (!*p_open) return;
    
    ImGui::SetNextWindowSize(ImVec2(700, 450), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Select Audio File", p_open)) {
        // Path bar
        ImGui::Text("Path: %s", currentPath.string().c_str());
        ImGui::Separator();
        
        // Parent directory button
        if (ImGui::Button("..")) {
            NavigateTo(currentPath.parent_path());
        }
        
        // File list
        ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
        
        try {
            std::vector<fs::directory_entry> entries;
            
            // Collect and sort entries
            for (const auto& entry : fs::directory_iterator(currentPath)) {
                entries.push_back(entry);
            }
            
            // Sort directories first, then files
            std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
                if (a.is_directory() != b.is_directory()) {
                    return a.is_directory();
                }
                return a.path().filename() < b.path().filename();
            });
            
            // Display entries
            for (const auto& entry : entries) {
                const auto& path = entry.path();
                std::string filename = path.filename().string();
                
                // Skip hidden files
                if (filename[0] == '.') continue;
                
                if (entry.is_directory()) {
                    // Directory
                    std::string label = "[DIR] " + filename;
                    if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            NavigateTo(path);
                        }
                    }
                } else if (entry.is_regular_file() && HasValidExtension(path)) {
                    // Audio file
                    std::string label = filename;
                    
                    // Add file size
                    auto size = entry.file_size();
                    if (size < 1024) {
                        label += " (" + std::to_string(size) + " B)";
                    } else if (size < 1024 * 1024) {
                        label += " (" + std::to_string(size / 1024) + " KB)";
                    } else {
                        label += " (" + std::to_string(size / (1024 * 1024)) + " MB)";
                    }
                    
                    if (ImGui::Selectable(label.c_str(), path.string() == selectedFile, ImGuiSelectableFlags_AllowDoubleClick)) {
                        selectedFile = path.string();
                        if (ImGui::IsMouseDoubleClicked(0)) {
                            onFileSelected(selectedFile);
                            *p_open = false;
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Error: %s", e.what());
        }
        
        ImGui::EndChild();
        
        ImGui::Separator();
        
        // Selected file and buttons
        if (!selectedFile.empty()) {
            ImGui::Text("Selected: %s", fs::path(selectedFile).filename().string().c_str());
            ImGui::SameLine();
            
            if (ImGui::Button("Open")) {
                onFileSelected(selectedFile);
                *p_open = false;
            }
            ImGui::SameLine();
        }
        
        if (ImGui::Button("Cancel")) {
            *p_open = false;
        }
    }
    ImGui::End();
}