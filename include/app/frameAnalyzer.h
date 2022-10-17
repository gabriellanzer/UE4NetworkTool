#pragma once

// StdLib Includes
#include <string>
#include <vector>

// Third party dependencies
#include <taskflow/core/taskflow.hpp>
#include <taskflow/core/executor.hpp>

// Using directives
using string = std::string;
template <typename T>
using vector = std::vector<T>;

struct FrameTexture
{
    string ImageName;
    unsigned int GlTexture;
    int Width, Height;
    float Duration = 0.016f;
};

struct LoadImageJob
{
	string ImagePath;
	void* ImageData = nullptr;
	int Width, Height, NumComp;
	bool FinishedLoading = false;
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

	tf::Taskflow _taskFlow;
	tf::Executor _taskExecutor;

	// Queue for OpenGL main-thread texture upload
	std::mutex _uploadImageLock;
	std::vector<LoadImageJob> _uploadImageBuff;
	std::deque<LoadImageJob> _uploadImageDeque;
	// Up to 64 simultaneous load image jobs
	std::array<LoadImageJob, 64> _loadImagePipeData;

    GLFWwindow* _window = nullptr;
    bool _autoPlay = true;
    float _playbackSpeed = 100.0f;
    int _imageOffset = 0;
    int _loadFrameIt = 0;
    vector<string> _loadFramesList;
    vector<FrameTexture> _textures;
};