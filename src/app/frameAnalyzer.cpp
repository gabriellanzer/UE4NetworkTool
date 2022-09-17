#include <app/frameAnalyzer.h>

// Third Party Includes
#include <fmt/core.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <imgui/imgui.h>
#include <pfd.h>
#include <stb_image.h>

// Internal Includes
#include <RVCore/utils.h>

FrameAnalyzerWindow::~FrameAnalyzerWindow()
{
    // We don't own the GLFW window
    // We only use it for inputs
    _window = nullptr;

    // Release Graphics API textures
    for (const SnapshotTexture& texture : _textures)
    {
	glDeleteTextures(1, &texture.glTexture);
    }

    _loadFramesList.clear();
    _textures.clear();
}

void FrameAnalyzerWindow::DrawMenuBarFileItems()
{
    if (ImGui::MenuItem("Import Frame Snapshot(s)"))
    {
	ImportFrameSnapshots();
    }
}

void FrameAnalyzerWindow::DrawMenuBarWindowItems()
{
    if (ImGui::MenuItem("Frame Analyzer"))
    {
	ShouldShow = true;
    }
}

void FrameAnalyzerWindow::Draw(ImGuiID dockSpaceId, double deltaTime)
{
    if (!ShouldShow) return;

    ImGui::SetNextWindowDockID(dockSpaceId, ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Frame Analyzer", &ShouldShow))
    {
	if (_textures.empty())
	{
	    ImGuiViewport* viewport = ImGui::GetMainViewport();

	    // Centralize text
	    const char* importText = "Import Frames to Start Using the Analyzer";
	    ImVec2 textSize = ImGui::CalcTextSize(importText);
	    ImVec2 textPos = ImGui::GetWindowSize();
	    textPos.x = (textPos.x - textSize.x) * 0.5f;
	    textPos.y = (textPos.y - textSize.y) * 0.5f;
	    // Draw Usage Text
	    ImGui::SetCursorPos(textPos);
	    ImGui::Text("%s", importText);

	    // Centralize Import Button
	    const char* btnText = "Import Frame Snapshot(s)";
	    float btnWidth = ImGui::CalcTextSize(btnText).x;
	    btnWidth += ImGui::GetStyle().FramePadding.x;
	    float btnPosX = ImGui::GetWindowWidth();
	    btnPosX = (btnPosX - btnWidth) * 0.5f;
	    ImGui::SetCursorPosX(btnPosX);
	    // Draw Import Button
	    if (ImGui::Button(btnText))
	    {
		ImportFrameSnapshots();
	    }

	    ImGui::End();
	    return;
	}

	// Try to show any available snapshot in timeline
	SnapshotTexture& texture = _textures[_imageOffset];

	// Check if we should increment imageOffset based on input
	static double playbackTime = 0;
	playbackTime += deltaTime * (_playbackSpeed / 100.0f);

	static int lastSpaceState = glfwGetKey(_window, GLFW_KEY_SPACE);
	static int lastRightState = glfwGetKey(_window, GLFW_KEY_RIGHT);
	static int lastLeftState = glfwGetKey(_window, GLFW_KEY_LEFT);

	const int size = static_cast<int>(_textures.size());
	if (_autoPlay)
	{
	    if (!lastSpaceState && glfwGetKey(_window, GLFW_KEY_SPACE))
	    {
		_autoPlay = false;
	    }

	    if (playbackTime > texture.duration)
	    {
		_imageOffset = (_imageOffset + 1) % size;
		playbackTime = 0;
	    }
	}
	else
	{
	    const int offset = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) ? 10 : 1;
	    if (!lastSpaceState && glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS)
	    {
		_autoPlay = true;
	    }
	    if (!lastRightState && glfwGetKey(_window, GLFW_KEY_RIGHT))
	    {
		_imageOffset = (_imageOffset + offset) % size;
	    }
	    if (!lastLeftState && glfwGetKey(_window, GLFW_KEY_LEFT))
	    {
		_imageOffset = (_imageOffset + size - offset) % size;
	    }
	}

	lastSpaceState = glfwGetKey(_window, GLFW_KEY_SPACE);
	lastRightState = glfwGetKey(_window, GLFW_KEY_RIGHT);
	lastLeftState = glfwGetKey(_window, GLFW_KEY_LEFT);

	// Draw one of the image frames
	ImGui::BeginChild("SnapshotViewer", ImVec2(), false);

	// Compute available size
	ImVec2 availCanvasSize = ImGui::GetContentRegionAvail();
	float sliderHeight = ImGui::CalcTextSize("##dummy", nullptr, true).y;
	sliderHeight += ImGui::GetStyle().FramePadding.y * 2.0f;
	availCanvasSize.y -= sliderHeight * 2;			      // Two sliders
	availCanvasSize.y -= ImGui::GetStyle().FramePadding.y * 2.0f; // Its own frame padding

	// Maintain aspect-ratio and fit by touching the corners from within
	ImVec2 imageDrawSize = availCanvasSize;
	float hAlignOffset = 0.0f; // Horizontal alignment offset
	float imageAspect = texture.width / (float)texture.height;
	float availAspect = availCanvasSize.x / (float)availCanvasSize.y;
	if (availAspect > imageAspect) // Canvas is wider than image (fit by height)
	{
	    imageDrawSize.x = availCanvasSize.y * imageAspect;
	    hAlignOffset = (availCanvasSize.x - imageDrawSize.x) / 2.0f;
	}
	else // Canvas is taller than image (fit by width)
	{
	    imageDrawSize.y = availCanvasSize.x / imageAspect;
	}

	// Ensure the image is centered if it has any gaps because of aspect correction
	ImGui::SetCursorPosX(hAlignOffset);
	ImGui::Image(reinterpret_cast<void*>(texture.glTexture), imageDrawSize);

	ImGui::SetNextItemWidth(availCanvasSize.x);
	ImGui::SliderInt("##_imageOffsetSlider", &_imageOffset, 0, size - 1, "Frame %d");
	ImGui::SetNextItemWidth(availCanvasSize.x);
	ImGui::DragFloat("##_playbackSpeedSlider", &_playbackSpeed, 1.0f, 1.0f, 1000.0f, "Playback Speed %.2f%%");
	ImGui::EndChild();
    }
    ImGui::End();
}

void FrameAnalyzerWindow::DrawLoadingFramesModal()
{
    if (_loadFramesList.empty()) return;

    // This is our first time showing the modal, trigger it up
    if (_loadFrameIt == 0)
    {
	ImGui::OpenPopup("Loading Frames");
    }

    // Show loading modal at center
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Loading Frames", nullptr, flags))
    {
	// Fetch currently loading snapshot path
	const string& snapshotPath = _loadFramesList[_loadFrameIt];
	_loadFrameIt++;

	size_t loadingListSize = _loadFramesList.size();

	// Display loading progress bar
	ImGui::ProgressBar(_loadFrameIt / (float)loadingListSize);

	// Center-aligned text feedback
	auto windowWidth = ImGui::GetWindowSize().x;
	string progressText = fmt::format("Progress {0}/{1}", _loadFrameIt, loadingListSize);
	auto textWidth = ImGui::CalcTextSize(progressText.c_str()).x;
	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::Text("%s", progressText.c_str());

	// Parse file-name, extension and directory
	string fileName, fileExt;
	string directory = rv::splitFilename(snapshotPath, fileName, fileExt);

	// Load image from disk
	int w, h, comp;
	unsigned char* image = stbi_load(snapshotPath.c_str(), &w, &h, &comp, STBI_rgb);
	if (image == nullptr)
	{
	    // Error handling
	    pfd::message openImageDialog("Open Image Error", fmt::format("Couldn't Load Image: {0}", snapshotPath),
					 pfd::choice::ok, pfd::icon::error);
	    return;
	}

	// We got ourselves a frame (if the analyzer is not open, do it now)
	ShouldShow = true;

	// Generate OpenGL image handle
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	unsigned int textureId;
	glGenTextures(1, &textureId);

	// Upload image to OpenGL
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Free RAM resources
	stbi_image_free(image);

	// Hold snapshot resource
	SnapshotTexture snapshot = {fileName, textureId, w, h};
	_textures.push_back(snapshot);

	// Close popup condition
	if (_loadFrameIt == loadingListSize)
	{
	    ImGui::CloseCurrentPopup();

	    // Clear loading list
	    _loadFramesList.clear();
	}

	ImGui::EndPopup();
    }
}
void FrameAnalyzerWindow::ImportFrameSnapshots()
{
    const string title = "Import Frame Snapshot(s)";
    const auto filters = vector<string>(
	{"Image File(s) (*.jpeg,*.png,*.tga,*.bmp,*.gif)", "*.jpeg;*.png;*.tga;*.bmp;*.gif", "All Files", "*"});
    const pfd::opt options = pfd::opt::multiselect;
    _loadFramesList = pfd::open_file(title, "C:/", filters, options).result();

    // Reset load iterator
    _loadFrameIt = 0;

    // Reserve texture slots, the load list will be loaded 1 image per frame
    _textures.reserve(_textures.size() + _loadFramesList.size());
}
