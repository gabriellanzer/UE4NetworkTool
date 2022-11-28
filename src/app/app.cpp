
// StdLib Includes
#include "imgui/imgui.h"
#include <corecrt_math.h>
#include <vector>

// Third Party Includes
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <implot/implot.h>
#include <pfd.h>

// Application Specific Includes
#include <app/app.h>
#include <app/settings.h>
#include <app/frameAnalyzer.h>
#include <app/serverLauncher.h>
#include <app/unrealAssetBrowser.h>

// Using directives
using string = std::string;
template <typename T>
using vector = std::vector<T>;

static bool showDemoWindows = false;

void UE4NetworkTool::Setup()
{
	// Setup Graphics APIs
	glfwSetErrorCallback(OnGlfwErrorCallback);
	if (!glfwInit())
	{
		return;
	}

	_window = glfwCreateWindow(1920, 1080, GetName(), nullptr, nullptr);
	if (!_window)
	{
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(_window);
	glfwSwapInterval(1);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glfwSetInputMode(_window, GLFW_STICKY_KEYS, GLFW_TRUE);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// ImGui::StyleColorsClassic();
	// ImGui::StyleColorsLight();
	// Setup Platform/Renderer backends
	const char* glsl_version = "#version 330";
	ImGui_ImplGlfw_InitForOpenGL(_window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Operating System Window Settings
	glfwSetWindowSizeLimits(_window, 640, 480, GLFW_DONT_CARE, GLFW_DONT_CARE);
	glfwSetWindowSizeCallback(_window, OnGlfwWindowResizeCallback);
	glfwMaximizeWindow(_window);

	// Initialize application settings handler
	// This has to happen *after* ImGUI context is created
	Settings::Init();

	// App specific allocation
	_serverLauncherWindow = new ServerLauncherWindow(true);
	_frameAnalyzerWindow = new FrameAnalyzerWindow(true, _window);

	// Test Unreal Asset Browser
	_unrealAssetBrowser = new UnrealAssetBrowser();
	// _unrealAssetBrowser->LoadAssetInfos("D:\\Aquiris\\wc2\\Content\\");
}

void UE4NetworkTool::Awake() { }

void UE4NetworkTool::Sleep() { }

void UE4NetworkTool::Shutdown()
{
	// App specific cleanup
	delete _serverLauncherWindow;
	delete _frameAnalyzerWindow;

	// ImGui shutdown
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();

	// Deinitialize the settings pointers
	// This has to happen *after* ImGUI context is destroyed
	// the context destruction will serialize the settings
	Settings::Deinit();

	// Graphics API shutdown
	glfwDestroyWindow(_window);
	glfwTerminate();
}

constexpr float fixedFPSDeltaTime = 1.0f/60.0f;

void UE4NetworkTool::Update(double deltaTime)
{
	shouldQuit = glfwWindowShouldClose(_window);

	_accDeltaTime += deltaTime;
	if (_accDeltaTime < fixedFPSDeltaTime && _fixedFPS) return;
	if (_accDeltaTime > fixedFPSDeltaTime)
	{
		_accDeltaTime = fmod(_accDeltaTime, fixedFPSDeltaTime);
	}

	// Poll first so ImGUI has the events.
	// This performs some callbacks as well
	glfwPollEvents();

	// ImGUI frame initialization
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	DrawAppScreen(_fixedFPS ? fixedFPSDeltaTime : deltaTime);

	// ImGUI frame baking (finalization)
	ImGui::Render();

	// GL Rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(_window);
}

void UE4NetworkTool::DrawAppScreen(double deltaTime)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			_frameAnalyzerWindow->DrawMenuBarFileItems();

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Windows"))
		{
			_serverLauncherWindow->DrawMenuBarWindowItems();
			_frameAnalyzerWindow->DrawMenuBarWindowItems();

			if (ImGui::MenuItem("Show Demo Windows"))
			{
				showDemoWindows = true;
			}
			ImGui::EndMenu();
		}

		if (!_unrealAssetBrowser->LoadAssetHandle._Is_ready())
		{
			ImGui::TextUnformatted("Loading Assets");
		}
		else
		{
			if (ImGui::Button("Print Assets"))
			{
				_unrealAssetBrowser->PrintRaceAssets();
			}
		}

		ImGui::Checkbox("Fix FPS", &_fixedFPS);

		ImGui::Text("%d FPS (%.2fms)", static_cast<int>(1.0 / deltaTime), deltaTime * 1000.0);
		ImGui::EndMainMenuBar();
	}

	// Create viewport docking space
	ImGuiID dockSpaceId = ImGui::DockSpaceOverViewport();

	// Create a loading modal if we got anything to load
	_frameAnalyzerWindow->DrawLoadingFramesModal();

	// Draw Windows
	_frameAnalyzerWindow->Draw(dockSpaceId, deltaTime);
	_serverLauncherWindow->Draw(dockSpaceId, deltaTime);

	if (showDemoWindows)
	{
		ImGui::ShowDemoWindow(&showDemoWindows);
		ImPlot::ShowDemoWindow(&showDemoWindows);
	}
}
