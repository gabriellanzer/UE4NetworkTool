
// StdLib Includes
#include <vector>

// Third Party Includes
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <implot/implot.h>
#include <pfd.h>

// Ravine Includes
#include <RVCore/projectManager.h>
#include <RVCore/utils.h>

// Application Specific Includes
#include <app/app.h>
#include <app/frameAnalyzer.h>
#include <app/serverLauncher.h>

// Using directives
using string = std::string;
template <typename T>
using vector = std::vector<T>;

static bool showDemoWindows = false;

void UE4ServerBootstrap::Setup()
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

    // App specific allocation
    _serverLauncherWindow = new ServerLauncherWindow(true);
    _frameAnalyzerWindow = new FrameAnalyzerWindow(true, _window);
}

void UE4ServerBootstrap::Awake() {}

void UE4ServerBootstrap::Sleep() {}

void UE4ServerBootstrap::Shutdown()
{
    // App specific cleanup
    delete _frameAnalyzerWindow;
    delete _serverLauncherWindow;

    // ImGui shutdown
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    // Graphics API shutdown
    glfwDestroyWindow(_window);
    glfwTerminate();
}

void UE4ServerBootstrap::Update(double deltaTime)
{
    shouldQuit = glfwWindowShouldClose(_window);

    // Poll first so ImGUI has the events.
    // This performs some callbacks as well
    glfwPollEvents();

    // ImGUI frame initialization
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawAppScreen(deltaTime);

    // ImGUI frame baking (finalization)
    ImGui::Render();

    // GL Rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(_window);
}

void UE4ServerBootstrap::DrawAppScreen(double deltaTime)
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

	ImGui::Text("FPS %d (%.2fms)", static_cast<int>(1.0 / deltaTime), deltaTime * 1000.0);
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
