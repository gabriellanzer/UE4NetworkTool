#include <app/frameAnalyzer.h>

// Third Party Includes
#include <fmt/core.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <imgui/imgui.h>
#include <pfd.h>
#include <stb_image.h>
#include <string.h>
#include <taskflow/algorithm/pipeline.hpp>

// Internal Includes
#include <RVCore/utils.h>
#include <utility>

FrameAnalyzerWindow::~FrameAnalyzerWindow()
{
	// We don't own the GLFW window
	// We only use it for inputs
	_window = nullptr;

	// Release Graphics API textures
	for (const FrameTexture& texture : _textures)
	{
		glDeleteTextures(1, &texture.TextureId);
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
		FrameTexture& texture = _textures[_imageOffset];

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

			if (playbackTime > texture.Duration)
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
		availCanvasSize.y -= sliderHeight * 2;						  // Two sliders
		availCanvasSize.y -= ImGui::GetStyle().FramePadding.y * 2.0f; // Its own frame padding

		// Maintain aspect-ratio and fit by touching the corners from within
		ImVec2 imageDrawSize = availCanvasSize;
		float hAlignOffset = 0.0f; // Horizontal alignment offset
		float imageAspect = texture.Width / (float)texture.Height;
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
		ImGui::Image(reinterpret_cast<void*>((intptr_t)texture.TextureId), imageDrawSize);

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
	if (_loadFramesList.empty() && _syncImageUploadQueue.empty()) return;

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
		size_t loadingListSize = _loadFramesList.size();

		// We only flush if there are more than 32 texture in the buffer or there are less than 32 textures
		// remaining. This ensures we don't keep locking the async tasks too often to pull from the buffer.
		if (_uploadImageBuff.size() > 32 || (loadingListSize - _loadFrameIt) <= 32)
		{
			_uploadImageLock.lock();
			_uploadImageDeque.insert(_uploadImageDeque.end(), _uploadImageBuff.begin(), _uploadImageBuff.end());
			_uploadImageBuff.clear();
			_uploadImageLock.unlock();
		}

		if (!_uploadImageDeque.empty())
		{
			constexpr int jobsMaxCount = 32;
			const int jobsCount = min(jobsMaxCount, (int)_uploadImageDeque.size());
			LoadImageJob uploadJobs[jobsMaxCount];

			for (int jobsIt = 0; jobsIt < jobsCount; ++jobsIt)
			{
				uploadJobs[jobsIt] = std::forward<LoadImageJob>(_uploadImageDeque.front());
				_uploadImageDeque.pop_front();
			}

			// Filter for failed load jobs
			int uploadCount = 0;
			for (int jobsIt = 0; jobsIt < jobsCount; ++jobsIt)
			{
				// We increment the load iterator regardless of loading
				// success as it is used to track operation completness
				++_loadFrameIt;

				// Check for failed jobs
				LoadImageJob& job = uploadJobs[jobsIt];
				if (job.ImageData == nullptr)
				{
					// Error handling
					pfd::message openImageDialog("Open Image Error",
						fmt::format("Couldn't Load Image: {0}", job.ImagePath), pfd::choice::ok, pfd::icon::error);
					continue;
				}

				// Only those that didn't fail are uploaded
				++uploadCount;
			}

			// Generate OpenGL image handle
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			unsigned int pixelBuffIds[jobsMaxCount];
			glGenBuffers(uploadCount, pixelBuffIds);

			unsigned int textureIds[jobsMaxCount];
			glGenTextures(uploadCount, textureIds);

			// Upload CPU Data to Pixel Buffer Objects (for async transfer)
			for (int jobsIt = 0, uploadIt = 0; jobsIt < jobsCount; ++jobsIt)
			{
				LoadImageJob& job = uploadJobs[jobsIt];
				if (job.ImageData == nullptr) continue;

				// Upload It only increments for valid upload jobs
				unsigned int pixelBuffId = pixelBuffIds[uploadIt];
				unsigned int textureId = textureIds[uploadIt];
				uploadIt++;

				// We got ourselves a frame (if the analyzer is not open, do it now)
				ShouldShow = true;

				// Upload image to Pixel Buffer Object
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBuffId);
				const int textureSize = job.Width * job.Height * 3;
				glBufferData(GL_PIXEL_UNPACK_BUFFER, textureSize, 0, GL_STREAM_DRAW);

				// map the buffer object into client's memory
				void* gpuAsyncData = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, textureSize,
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
				if (gpuAsyncData)
				{
					// Copy image memory to async stream buffer
					memcpy(gpuAsyncData, job.ImageData, textureSize);

					// Release the mapped buffer
					glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
				}

				// Free CPU memory resources
				stbi_image_free(job.ImageData);

				// Enqueu image for upload sync
				_syncImageUploadQueue.emplace(std::forward<string>(job.ImagePath), job.Width, job.Height, pixelBuffId,
					textureId);
			}

			// Unbind any used OpenGL object
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Every frame we sync a number of frames from the sync queue
		if (!_syncImageUploadQueue.empty())
		{
			constexpr int syncMaxCount = 1;
			const int syncCount = min(syncMaxCount, (int)_syncImageUploadQueue.size());
			for (int syncIt = 0; syncIt < syncCount; syncIt++)
			{
				const SyncImageUploadJob& job = _syncImageUploadQueue.front();

				glBindTexture(GL_TEXTURE_2D, job.TextureId);
				glBindBuffer(GL_PIXEL_UNPACK_BUFFER, job.PixelBuffId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, job.Width, job.Height, 0, GL_RGB, GL_UNSIGNED_BYTE,
					0);

				// Parse file-name, extension and directory
				string fileName, fileExt;
				string directory = rv::splitFilename(job.ImagePath, fileName, fileExt);

				// Hold snapshot resource
				FrameTexture snapshot = {std::forward<string>(fileName), job.TextureId, job.Width, job.Height};
				_textures.push_back(snapshot);
				_syncImageUploadQueue.pop();

				// Release Pixel Buffer Object
				glDeleteBuffers(1, &job.PixelBuffId);
			}

			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Display loading progress bar
		ImGui::ProgressBar(_loadFrameIt / (float)loadingListSize);

		// Center-aligned text feedback
		auto windowWidth = ImGui::GetWindowSize().x;
		string progressText = fmt::format("Progress {0}/{1}", _loadFrameIt, loadingListSize);
		auto textWidth = ImGui::CalcTextSize(progressText.c_str()).x;
		ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
		ImGui::Text("%s", progressText.c_str());

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

	// Use taskflow to create a parallel loading stage pipeflow with a serial thread-locked queued one
	static tf::Pipeline _loadImagePipeline = tf::Pipeline(16,
		tf::Pipe {tf::PipeType::SERIAL,
			[&](tf::Pipeflow& pf)
			{
				// Stop emitting tokens if we reached loading queue end
				if (pf.token() == _loadFramesList.size())
				{
					pf.stop();
					return;
				}

				// Enqueue the image to be loaded from the pipe
				_loadImagePipeData[pf.line()] = {_loadFramesList[pf.token()]};
			}},
		tf::Pipe {tf::PipeType::PARALLEL,
			[&](tf::Pipeflow& pf)
			{
				LoadImageJob& job = _loadImagePipeData[pf.line()];
				job.ImageData = stbi_load(job.ImagePath.c_str(), &job.Width, &job.Height, &job.NumComp, STBI_rgb);
				job.NumComp = STBI_rgb;
			}},
		tf::Pipe {tf::PipeType::SERIAL, [&](tf::Pipeflow& pf)
			{
				LoadImageJob& job = _loadImagePipeData[pf.line()];
				_uploadImageLock.lock();
				_uploadImageBuff.push_back(std::forward<LoadImageJob>(job));
				_uploadImageLock.unlock();
			}});
	_taskFlow.composed_of(_loadImagePipeline).name("pipeline");
	_taskExecutor.run(_taskFlow);
}
