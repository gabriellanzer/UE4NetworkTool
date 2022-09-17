#ifndef __APP__H__
#define __APP__H__

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <RVCore/iapp.h>

//struct SnapshotTexture
//{
//    std::string imageName;
//    unsigned int glTexture;
//    int width, height;
//    float duration = 0.016f;
//};

class UE4ServerBootstrap : public rv::IApp
{
  public:
    UE4ServerBootstrap() : rv::IApp("UE4 Server Bootstrap"){};
    ~UE4ServerBootstrap() = default;

    virtual void Awake() override;
    virtual void Setup() override;
    virtual void Shutdown() override;
    virtual void Sleep() override;
    virtual void Update(double deltaTime) override;

  private:
    static void OnGlfwErrorCallback(int error, const char* description)
    {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    static void OnGlfwWindowResizeCallback(GLFWwindow* window, int width, int height)
    {
	glViewport(0, 0, width, height);
    }

    UE4ServerBootstrap(UE4ServerBootstrap&&) = delete;
    UE4ServerBootstrap(const UE4ServerBootstrap&) = delete;
    UE4ServerBootstrap& operator=(UE4ServerBootstrap&&) = delete;
    UE4ServerBootstrap& operator=(const UE4ServerBootstrap&) = delete;

    void DrawAppScreen(double deltaTime);
    GLFWwindow* _window{nullptr};

    // Windows
    class ServerLauncherWindow* _serverLauncherWindow;
    class FrameAnalyzerWindow* _frameAnalyzerWindow;
};

#endif //!__APP__H__