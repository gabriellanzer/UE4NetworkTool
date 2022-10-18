#ifndef __APP__H__
#define __APP__H__

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <RVCore/iapp.h>

class UE4NetworkTool : public rv::IApp
{
  public:
	UE4NetworkTool()
		: rv::IApp("UE4 Network Tool"), _serverLauncherWindow(nullptr), _frameAnalyzerWindow(nullptr),
		  _unrealAssetBrowser(nullptr){};
	~UE4NetworkTool() override = default;
	UE4NetworkTool(UE4NetworkTool&&) = delete;
	UE4NetworkTool(const UE4NetworkTool&) = delete;
	UE4NetworkTool& operator=(UE4NetworkTool&&) = delete;
	UE4NetworkTool& operator=(const UE4NetworkTool&) = delete;

	void Awake() override;
	void Setup() override;
	void Shutdown() override;
	void Sleep() override;
	void Update(double deltaTime) override;

  private:
	static void OnGlfwErrorCallback(int error, const char* description)
	{
		fprintf(stderr, "Glfw Error %d: %s\n", error, description);
	}

	static void OnGlfwWindowResizeCallback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void DrawAppScreen(double deltaTime);
	GLFWwindow* _window{nullptr};

	// Windows
	class ServerLauncherWindow* _serverLauncherWindow;
	class FrameAnalyzerWindow* _frameAnalyzerWindow;
	class UnrealAssetBrowser* _unrealAssetBrowser;
};

#endif //!__APP__H__