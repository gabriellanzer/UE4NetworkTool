#ifndef __APP__H__
#define __APP__H__

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <RVCore/iapp.h>

class UE4ServerBootstrap : public rv::IApp
{
  public:
	UE4ServerBootstrap()
		: rv::IApp("UE4 Server Bootstrap"), _serverLauncherWindow(nullptr), _frameAnalyzerWindow(nullptr){};
	~UE4ServerBootstrap() override = default;
	UE4ServerBootstrap(UE4ServerBootstrap&&) = delete;
	UE4ServerBootstrap(const UE4ServerBootstrap&) = delete;
	UE4ServerBootstrap& operator=(UE4ServerBootstrap&&) = delete;
	UE4ServerBootstrap& operator=(const UE4ServerBootstrap&) = delete;

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
};

#endif //!__APP__H__