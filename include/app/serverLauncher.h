#pragma once

typedef unsigned int ImGuiID;

#if WIN32
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>
#include <vector>
#endif

class ServerLauncherWindow
{
  public:
    ServerLauncherWindow() = default;
    ~ServerLauncherWindow();
    explicit ServerLauncherWindow(bool isOpen) : ShouldShow(isOpen){};

    void Draw(ImGuiID dockSpaceId, double deltaTime);
    void DrawMenuBarWindowItems();

    bool ShouldShow = false;

  private:
#if WIN32
    void LaunchServerProcess();
    void ForceCloseServer();
    void PullServerOutputLog();

    PPROCESS_INFORMATION _serverProcInfo = nullptr;
    HANDLE _hChildStdOut_Rd = nullptr;
    HANDLE _hChildStdOut_Wr = nullptr;
    HANDLE _hAsyncReadServerHandle = nullptr;
    std::vector<char> _asyncReadQueue;
    std::vector<std::string> _serverLogs;

    // Statics
    DWORD static __stdcall AsyncReadServerStdout(void* pVoid);
    static std::mutex _threadMutex;
#endif
};