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
template <typename T>
using vector = std::vector<T>;
using string = std::string;
typedef unsigned int ImGuiID;

enum class LogVerbosity : unsigned char
{
	Fatal,
	Error,
	Warning,
	Display,
	Log,
	Verbose,
	VeryVerbose
};

struct LogEntry
{
	string message;
	LogVerbosity verbosity;
};

class ServerLauncherWindow
{
  public:
	ServerLauncherWindow() = delete;
	explicit ServerLauncherWindow(bool isOpen);
	~ServerLauncherWindow();

	void Draw(ImGuiID dockSpaceId, double deltaTime);
	void DrawMenuBarWindowItems();

	bool ShouldShow = false;
	bool FirstTimeOpen = true;

  private:
	void LoadSettings();
	void StoreSettings();

	string* _settingsEntry = nullptr;
	int _scrollTargetParamId = 0;
	int _selectedParamId = 0;
	char _uprojectBuf[512];
	string _uprojectFileName;
	char _paramBuf[512];
	vector<string> _launchParams;
	char _additionalParamBuf[512];
	string _additionalParamLine;

	bool _forceAutoScroll = {false};
	vector<LogEntry> _serverLogs;

#if WIN32
	void LaunchServerProcess();
	DWORD ForceCloseServer();
	void PullServerOutputLog();
	void PullServerProcessStatus();

	DWORD _exitCode = 0;
	PPROCESS_INFORMATION _serverProcInfo = nullptr;
	HANDLE _hChildStdOut_Rd = nullptr;
	HANDLE _hChildStdOut_Wr = nullptr;
	HANDLE _hAsyncReadServerHandle = nullptr;
	bool _copyToClipboard = false;
	string _partialLogLine;
	vector<char> _asyncReadQueue;

	// Statics
	static void CloseServerOnCrashCallback(void* pVoid);
	DWORD static __stdcall AsyncReadServerStdout(void* pVoid);
	static std::mutex _threadMutex;
#endif
};