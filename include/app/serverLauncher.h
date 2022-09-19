#pragma once

// StdLib Includes
#include <string>
#include <vector>

// Windows Server Deploy Specific Includes
#if WIN32
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>
#endif

// Using Directives and TypeDefs
template<typename T>
using vector = std::vector<T>;
using string = std::string;
typedef unsigned int ImGuiID;

class ServerLauncherWindow
{
  public:
    ServerLauncherWindow();
    ~ServerLauncherWindow();
    explicit ServerLauncherWindow(bool isOpen) : ShouldShow(isOpen){};

    void Draw(ImGuiID dockSpaceId, double deltaTime);
    void DrawMenuBarWindowItems();

    bool ShouldShow = false;

  private:
    int _scrollTargetParamId = 0;
    int _selectedParamId = 0;
    vector<string> _launchParams;

#if WIN32
    void LaunchServerProcess();
    void ForceCloseServer();
    void PullServerOutputLog();

    PPROCESS_INFORMATION _serverProcInfo = nullptr;
    HANDLE _hChildStdOut_Rd = nullptr;
    HANDLE _hChildStdOut_Wr = nullptr;
    HANDLE _hAsyncReadServerHandle = nullptr;
    vector<char> _asyncReadQueue;
    vector<string> _serverLogs;

    // Statics
    static int OnParamsInputTextCallback(struct ImGuiInputTextCallbackData* cbData);
    DWORD static __stdcall AsyncReadServerStdout(void* pVoid);
    static std::mutex _threadMutex;
#endif
};