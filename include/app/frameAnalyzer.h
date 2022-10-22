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

// Used to load image data from disk in parallel
struct LoadImageJob
{
	string ImagePath;
	void* ImageData = nullptr;
	int Width, Height, NumComp;
};

// Used to flush async gpu write alongside frames
struct SyncImageUploadJob
{
	SyncImageUploadJob(string path, int w, int h, int bufId, int texId)
		: ImagePath(path), Width(w), Height(h), PixelBuffId(bufId), TextureId(texId)
	{
	}

	string ImagePath;
	int Width, Height;
	unsigned int PixelBuffId;
	unsigned int TextureId;
};

// Used to keep track of and draw loaded frames
struct FrameTexture
{
	string ImageName;
	unsigned int TextureId;
	int Width, Height;
	float Duration = 0.016f;
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
	std::queue<SyncImageUploadJob> _syncImageUploadQueue;
	// Up to 16 simultaneous load image jobs
	std::array<LoadImageJob, 16> _loadImagePipeData;

	GLFWwindow* _window = nullptr;
	bool _autoPlay = true;
	float _playbackSpeed = 100.0f;
	int _imageOffset = 0;
	int _loadFrameIt = 0;
	vector<string> _loadFramesList;
	vector<FrameTexture> _textures;
};