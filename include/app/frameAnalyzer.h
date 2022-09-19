#pragma once

// StdLib Includes
#include <string>
#include <vector>

// Using directives
using string = std::string;
template <typename T>
using vector = std::vector<T>;

struct SnapshotTexture
{
    std::string imageName;
    unsigned int glTexture;
    int width, height;
    float duration = 0.016f;
};

struct GLFWwindow;
typedef unsigned int ImGuiID;

class FrameAnalyzerWindow
{
  public:
    FrameAnalyzerWindow(bool isOpen, GLFWwindow* window) : ShouldShow(isOpen), _window(window){};
    ~FrameAnalyzerWindow();

    void DrawMenuBarFileItems();
    void DrawMenuBarWindowItems();
    void Draw(ImGuiID dockSpaceId, double deltaTime);
    void DrawLoadingFramesModal();

    bool ShouldShow = false;

  private:
    void ImportFrameSnapshots();

    GLFWwindow* _window = nullptr;
    bool _autoPlay = true;
    float _playbackSpeed = 100.0f;
    int _imageOffset = 0;
    int _loadFrameIt = 0;
    vector<string> _loadFramesList;
    vector<SnapshotTexture> _textures;
};