#include <app/serverLauncher.h>

// Third Party Includes
#include <pfd.h>
#include <fmt/format.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#undef IMGUI_DEFINE_MATH_OPERATORS

// Internal Includes
#include <RVCore/utils.h>
#include <app/settings.h>

using std::string_view;

ServerLauncherWindow::ServerLauncherWindow(bool isOpen)
	: ShouldShow(isOpen), _settingsEntry(Settings::Register("ServerLauncherParams"))
{
	_paramBuf[0] = '\0';
	_additionalParamBuf[0] = '\0';
}

ServerLauncherWindow::~ServerLauncherWindow()
{
	// Set information to be saved upon closing
	StoreSettings();
	_launchParams.clear();
	_settingsEntry = nullptr;

#if WIN32
	// Close server-process if existing
	if (_serverProcInfo != nullptr)
	{
		ForceCloseServer();
	}
#endif
}

void ServerLauncherWindow::Draw(ImGuiID dockSpaceId, double deltaTime)
{
	if (!ShouldShow) return;

	if (FirstTimeOpen)
	{
		FirstTimeOpen = false;
		LoadSettings();
	}

	ImGui::SetNextWindowDockID(dockSpaceId, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Server Launcher", &ShouldShow))
	{
		ImGui::TextUnformatted("Launch Parameters");

		// Setup double-columns layout
		if (ImGui::BeginTable("Launch Parameters", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextColumn();

			// Draw parameters list
			if (ImGui::BeginListBox("##launchParamsList", ImVec2(-FLT_MIN, 0.0f)))
			{
				for (int i = 0; i < _launchParams.size(); i++)
				{
					bool isSelected = (_selectedParamId == i);
					string& paramEntry = _launchParams[i];

					ImGui::PushID(i);

					constexpr ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns |
														   ImGuiSelectableFlags_AllowItemOverlap |
														   ImGuiSelectableFlags_AllowDoubleClick;
					ImVec2 framePadding = ImGui::GetStyle().FramePadding;
					float vSize = framePadding.y * 2 + ImGui::GetTextLineHeight();
					if (ImGui::Selectable("##paramField", isSelected, flags, ImVec2(0.0f, vSize)))
					{
						if (_selectedParamId != i)
						{
							memcpy(_paramBuf, paramEntry.c_str(), paramEntry.size() + 1);
							_scrollTargetParamId = _selectedParamId = i;
						}
					}
					ImVec2 selMin = ImGui::GetItemRectMin();
					ImVec2 selMax = ImGui::GetItemRectMax();

					ImGui::SameLine();
					ImGui::Text("Param %d:", i);
					ImGui::SameLine();

					// Detect selection index even if the click was in the InputText
					ImGuiWindow* window = ImGui::GetCurrentWindow();
					float inputTextWidth = selMax.x - ImGui::GetCursorPosX() - framePadding.x * 3;
					ImVec2 frameSize = ImVec2(inputTextWidth, vSize);
					ImVec2 rectMin = window->DC.CursorPos;
					ImVec2 rectMax = window->DC.CursorPos + frameSize;
					if (ImGui::IsMouseHoveringRect(rectMin, rectMax))
					{
						// Update selection if clicked on InputText
						if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
						{
							memcpy(_paramBuf, paramEntry.c_str(), paramEntry.size() + 1);
							_scrollTargetParamId = _selectedParamId = i;
							isSelected = true;
						}

						// Manual highlight of selectable when hovering InputText
						const ImU32 col = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
						ImGui::GetWindowDrawList()->AddRectFilled(selMin, selMax, col);
					}

					// Selected InputText write to the _paramBuf
					if (isSelected)
					{
						ImGui::SetNextItemWidth(inputTextWidth);
						if (ImGui::InputText("##paramInput", _paramBuf, 512))
						{
							paramEntry = _paramBuf;
						}
					}
					else // Unselected are readonly of the stored entry values
					{
						ImGui::SetNextItemWidth(inputTextWidth);
						if (ImGui::InputText("##paramInput", paramEntry.data(), paramEntry.size() + 1,
											 ImGuiInputTextFlags_ReadOnly))
						{
							paramEntry = _paramBuf;
						}
					}

					// Ensure we focus the newly selected input field
					if (isSelected && i == _scrollTargetParamId)
					{
						// Reset scroll id so it doesn't keep pushing
						_scrollTargetParamId = -1;

						// Scroll to target if it is not visible
						ImGui::SetKeyboardFocusHere(-1);
					}
					ImGui::PopID();
				}

				ImGui::EndListBox();
			}

			// Side panel REST operations
			ImGui::TableNextColumn();
			ImGui::TextWrapped(
				"Info: The parameters are all appended in order and with '?' character between them (no spaces).");
			if (ImGui::Button("New Param"))
			{
				if (!_launchParams.empty())
				{
					_launchParams[_selectedParamId] = _paramBuf;
					_paramBuf[0] = '\0';
				}
				_selectedParamId = _launchParams.size();
				_launchParams.emplace_back("");
				_scrollTargetParamId = _selectedParamId;
			}
			ImGui::BeginDisabled(_launchParams.empty());
			if (ImGui::Button("Remove Selected"))
			{
				_launchParams.erase(_launchParams.begin() + _selectedParamId);
				if (!_launchParams.empty())
				{
					_selectedParamId = max(_selectedParamId - 1, 0);
					const auto& paramEntry = _launchParams[_selectedParamId];
					memcpy(_paramBuf, paramEntry.c_str(), paramEntry.size() + 1);
					_scrollTargetParamId = _selectedParamId;
				}
			}
			ImGui::EndDisabled();
			ImGui::EndTable();
		}

		// Additional parameter line for arguments as '-log -LogCmds'
		if (ImGui::InputText("Additional Parameter Line", _additionalParamBuf, 512))
		{
			_additionalParamLine = _additionalParamBuf;
		}

#if WIN32
		ImGui::BeginDisabled(_serverProcInfo != nullptr);
		if (ImGui::Button("Start Server"))
		{
			LaunchServerProcess();
		}
		ImGui::EndDisabled();
		ImGui::BeginDisabled(_serverProcInfo == nullptr);
		ImGui::SameLine();
		if (ImGui::Button("Kill Server"))
		{
			_exitCode = ForceCloseServer();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Copy to Clipboard"))
		{
			_copyToClipboard = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear Logs"))
		{
			_serverLogs.clear();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Force Auto-Scroll", &_forceAutoScroll);
		ImGui::SameLine();
		PullServerProcessStatus();
		PullServerOutputLog();
#endif
	}
	ImGui::End();
}

void ServerLauncherWindow::DrawMenuBarWindowItems()
{
	if (ImGui::MenuItem("Server Launcher"))
	{
		ShouldShow = true;
	}
}

void ServerLauncherWindow::LoadSettings()
{
	// Load Settings
	if (_settingsEntry->empty()) return;

	// Load launch parameters count
	string entryView = *_settingsEntry;

	size_t itPos = entryView.find_first_of('|');
	if (itPos == size_t(-1)) return;

	entryView[itPos] = '\0';
	char* tokenPtr = &entryView[0];

	int paramsCount = 0;
	if (sscanf_s(tokenPtr, "ParamsCount=%i", &paramsCount) != 1) return;

	// Load each parameter
	_launchParams.reserve(paramsCount);
	for (int i = 0; i < paramsCount; i++)
	{
		size_t itStart = itPos + 1;
		itPos = entryView.find_first_of('|', itStart);
		if (itPos == size_t(-1)) break;

		entryView[itPos] = _paramBuf[0] = '\0';
		tokenPtr = &entryView[itStart];
		sscanf_s(tokenPtr, "Param=%s", _paramBuf);
		_launchParams.push_back(string(_paramBuf));
	}

	// Override params buffer
	if (!_launchParams.empty())
	{
		string& paramEntry = _launchParams[0];
		memcpy(_paramBuf, paramEntry.c_str(), paramEntry.size() + 1);
	}

	// Fetch additional parameters line
	size_t itStart = itPos + 1;
	itPos = entryView.find_first_of('|', itStart);
	if (itPos == size_t(-1)) return;

	_additionalParamLine = std::forward<string>(entryView.substr(itStart, itPos - itStart));
	memcpy(_additionalParamBuf, _additionalParamLine.c_str(), _additionalParamLine.size() + 1);
}

void ServerLauncherWindow::StoreSettings()
{
	string saveStr = fmt::format("ParamsCount={0}|", _launchParams.size());
	for (const string& param : _launchParams)
	{
		saveStr += fmt::format("Param={0}|", param);
	}
	saveStr += fmt::format("{0}|", _additionalParamLine);
	if (_settingsEntry != nullptr)
	{
		_settingsEntry->assign(saveStr);
	}
}

#if WIN32
void ServerLauncherWindow::LaunchServerProcess()
{
	// Reset Exit flag
	_exitCode = 0;

	// Setup server pipes
	// Set the bInheritHandle flag so pipe handles are inherited.
	SECURITY_ATTRIBUTES saAttr;
	ZeroMemory(&saAttr, sizeof(saAttr));
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = true;
	saAttr.lpSecurityDescriptor = nullptr;

	// Create a pipe for the child process's STDOUT.
	if (!CreatePipe(&_hChildStdOut_Rd, &_hChildStdOut_Wr, &saAttr, 0))
	{
		pfd::message pipeCreationError("Create Pipe Error", "Couldn't create pipe: Stdout", pfd::choice::ok,
									   pfd::icon::error);
	}

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(_hChildStdOut_Rd, HANDLE_FLAG_INHERIT, 0))
	{
		pfd::message pipeCreationError("Create Pipe Error", "Couldn't set handle information on Stdout",
									   pfd::choice::ok, pfd::icon::error);
	}

	// Build launch command line
	string cmdLine;

	// Get UE4Editor Path
	size_t ue4PathSize = 0;
	char* ue4PathArr = nullptr;
	_dupenv_s(&ue4PathArr, &ue4PathSize, "UE_EDITOR_PATH");
	cmdLine = ue4PathArr;

	// Get current executable directory
	char appPath[_MAX_PATH + 1];
	GetModuleFileName(nullptr, appPath, _MAX_PATH);
	string fileName, fileExt;
	string directory = rv::splitFilename(string(appPath), fileName, fileExt);
	cmdLine += " " + directory; // " D:\\Aquiris\\wc2\\";

	// Append UPROJECT
	cmdLine += "HorizonChase2.uproject";

	// Append custom parameters
	cmdLine += " ";

	//	cmdLine += "/Game/HorizonChase2/Maps/Tracks/USA/USA_CA_SanFrancisco/L_USA_CA_SanFrancisco"
	//			   "?countdown=15?MaxPlayers=8";
	//	cmdLine += "?race=/Game/HorizonChase2/Maps/Tracks/USA/USA_CA_SanFrancisco/"
	//			   "DA_USA_CA_SanFrancisco_Race.DA_USA_CA_SanFrancisco_Race";
	for (int i = 0; i < _launchParams.size(); i++)
	{
		// Trim any white-spaces before and after
		string_view param = _launchParams[i];
		size_t iniL = param.find_first_not_of(' ');
		size_t iniR = param.find_last_not_of(' ');
		param = param.substr(iniL, iniR - iniL + 1);

		// Append parameter
		cmdLine += param;

		// Append separator
		if (i != _launchParams.size() - 1) cmdLine += "?";
	}

	// Launch server params (-stdout, so we can hook custom io pipes)
	cmdLine += " -server -stdout";

	// Last insertion of custom parameters line
	cmdLine += " " + _additionalParamLine;

	_serverProcInfo = new PROCESS_INFORMATION();
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE;

	// Set up members of the PROCESS_INFORMATION structure.
	ZeroMemory(_serverProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure.
	// This structure specifies the STDIN and STDOUT handles for redirection.
	ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.hStdError = _hChildStdOut_Wr;
	siStartInfo.hStdOutput = _hChildStdOut_Wr;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	// Create the child process.
	bSuccess = CreateProcessA(nullptr,
							  (TCHAR*)cmdLine.c_str(), // command line
							  nullptr,				   // process security attributes
							  nullptr,				   // primary thread security attributes
							  TRUE,					   // handles are inherited
							  0,					   // creation flags
							  nullptr,				   // use parent's environment
							  nullptr,				   // use parent's current directory
							  &siStartInfo,			   // STARTUPINFO pointer
							  _serverProcInfo);		   // receives PROCESS_INFORMATION

	// If an error occurs, exit the application.
	if (!bSuccess)
	{
		pfd::message launchErrorDialog("Server Launch Failed", "Couldn't launch server executable!", pfd::choice::ok,
									   pfd::icon::error);
		delete _serverProcInfo;
		_serverProcInfo = nullptr;
		return;
	}

	// Launch a thread to perform async loading of the data
	_hAsyncReadServerHandle = CreateThread(0, 0, AsyncReadServerStdout, this, 0, nullptr);

	// Assign process to job object which will auto-terminate the server when the app crashes
	HANDLE ghJob = CreateJobObject(nullptr, nullptr); // GLOBAL
	if (ghJob == nullptr)
	{
		pfd::message jobCreationErrorDialog("Job Object - Creation Failed",
											"Beware, if the app crashes the server process will NOT close!",
											pfd::choice::ok, pfd::icon::error);
	}

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {0};

	// Configure all child processes associated with the job to terminate when the
	jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!SetInformationJobObject(ghJob, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo)))
	{
		pfd::message jobSetInfoErrorDialog("Job Object - Set Info Failed",
										   "Beware, if the app crashes the server process will NOT close!",
										   pfd::choice::ok, pfd::icon::error);
	}

	if (!AssignProcessToJobObject(ghJob, _serverProcInfo->hProcess))
	{
		pfd::message jobAssignServerErrorDialog("Job Object - Assign Server Failed",
												"Beware, if the app crashes the server process will NOT close!",
												pfd::choice::ok, pfd::icon::error);
	}

	// Close handles to the stdout pipe no longer needed by the child process.
	// We have to close the stdout write handle in order to start reading from the other end.
	CloseHandle(_hChildStdOut_Wr);
}

DWORD ServerLauncherWindow::ForceCloseServer()
{
	if (_serverProcInfo == nullptr) return _exitCode;

	// Terminate server thread
	TerminateProcess(_serverProcInfo->hProcess, 0);
	DWORD exitCode = WaitForSingleObject(_serverProcInfo->hProcess, 1000);
	CloseHandle(_serverProcInfo->hProcess);
	CloseHandle(_serverProcInfo->hThread);

	// Terminate async read thread
	TerminateThread(_hAsyncReadServerHandle, 0);
	WaitForSingleObject(_hAsyncReadServerHandle, 1000);
	CloseHandle(_hAsyncReadServerHandle);

	// Invalidate process info
	delete _serverProcInfo;
	_serverProcInfo = nullptr;

	return exitCode;
}

static LogVerbosity ParseLogVerbosity(string_view logStrView)
{
	if (logStrView.find("Fatal") != -1) return LogVerbosity::Fatal;
	if (logStrView.find("Error") != -1) return LogVerbosity::Error;
	if (logStrView.find("Warning") != -1) return LogVerbosity::Warning;
	if (logStrView.find("Display") != -1) return LogVerbosity::Display;
	if (logStrView.find("Log") != -1) return LogVerbosity::Log;
	if (logStrView.find("Verbose") != -1) return LogVerbosity::Verbose;
	if (logStrView.find("VeryVerbose") != -1) return LogVerbosity::VeryVerbose;
	return LogVerbosity::Log;
}

constexpr ImVec4 LogColors[7] = {
	ImVec4(1.0f, 0.0f, 0.0f, 1.0f), // LogVerbosity::Fatal
	ImVec4(0.7f, 0.0f, 0.0f, 1.0f), // LogVerbosity::Error
	ImVec4(0.8f, 0.8f, 0.0f, 1.0f), // LogVerbosity::Warning
	ImVec4(0.9f, 0.9f, 0.9f, 1.0f), // LogVerbosity::Display
	ImVec4(0.8f, 0.8f, 0.8f, 1.0f), // LogVerbosity::Log
	ImVec4(0.7f, 0.7f, 0.7f, 1.0f), // LogVerbosity::Verbose
	ImVec4(0.6f, 0.6f, 0.6f, 1.0f), // LogVerbosity::VeryVerbose
};

void ServerLauncherWindow::PullServerOutputLog()
{
	static string output;
	{
		std::lock_guard<std::mutex> lock(_threadMutex);
		if (!_asyncReadQueue.empty())
		{
			output.assign(_asyncReadQueue.begin(), _asyncReadQueue.end());
			_asyncReadQueue.clear();
		}
	}

	// Push log entry
	if (!output.empty())
	{
		size_t iniEndLine = 0;
		size_t endEndLine = output.find_first_of('\n', iniEndLine);
		while (endEndLine != string::npos)
		{
			// Parse log entry
			size_t msgSize = endEndLine - iniEndLine;
			string msg = output.substr(iniEndLine, msgSize);
			string_view msgView = msg;

			// Parse verbosity
			LogVerbosity verbosity = LogVerbosity::Log;
			size_t verbIniP = -1;

			// Has time-stamp
			if (msgView[0] == '[')
			{
				// Skip time-stamp
				verbIniP = msgView.find_first_of(']', verbIniP + 1);
				// Skip log counter
				verbIniP = msgView.find_first_of(']', verbIniP + 1);
			}

			// Skip log category
			verbIniP = msgView.find_first_of(':', verbIniP + 1) + 1;
			size_t verbEndP = msgView.find_first_of(':', verbIniP + 1);
			if (verbIniP != -1 && verbEndP != -1)
			{
				string_view verbView = msgView.substr(verbIniP, verbEndP - verbIniP);
				verbosity = ParseLogVerbosity(verbView);
			}

			// Create log entry 
			_serverLogs.push_back({std::forward<string>(msg), verbosity});

			// Continue next-line parsing
			iniEndLine = endEndLine + 1;
			if (endEndLine == output.size() - 1) break;
			endEndLine = output.find_first_of('\n', iniEndLine);

			// No more endLines and we have a piece of log left
			if (endEndLine == -1)
			{
				_ASSERT(false);
				_partialLogLine = std::forward<string>(output.substr(iniEndLine, output.size() - iniEndLine + 1));
			}
		}
		output.clear();
	}

	ImGui::BeginChild("Server Output Log", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
	if (_copyToClipboard) ImGui::LogToClipboard();
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(_serverLogs.size()));
	while (clipper.Step())
	{
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
		{
			const LogEntry& logEntry = _serverLogs[i];
			ImGui::PushStyleColor(ImGuiCol_Text, LogColors[(int)logEntry.verbosity]);
			ImGui::TextUnformatted(logEntry.message.c_str());
			ImGui::PopStyleColor();

			// Auto-scroll
			if (ImGui::GetScrollY() == ImGui::GetScrollMaxY() || _forceAutoScroll)
			{
				ImGui::SetScrollHereY();
			}
		}
	}
	if (_copyToClipboard) ImGui::LogFinish();
	ImGui::PopStyleVar();
	ImGui::EndChild();
	_copyToClipboard = false;
}

void ServerLauncherWindow::PullServerProcessStatus()
{
	if (_serverProcInfo != nullptr)
	{
		if (!GetExitCodeProcess(_serverProcInfo->hProcess, &_exitCode))
		{
			ImGui::TextUnformatted("Server Status: Invalid Handle");
		}
	}

	if (_exitCode == STILL_ACTIVE)
	{
		ImGui::TextUnformatted("Server Status: Running");
	}
	else if (_exitCode != 0)
	{
		ImGui::Text("Server Status: Crashed (ExitCode=%lu)", _exitCode);
		ForceCloseServer();
	}
	else
	{
		ImGui::Text("Server Status: Not Running");
	}
}

std::mutex ServerLauncherWindow::_threadMutex;

void ServerLauncherWindow::CloseServerOnCrashCallback(void* pVoid)
{
	auto* window = static_cast<ServerLauncherWindow*>(pVoid);
	window->ForceCloseServer();
}

DWORD ServerLauncherWindow::AsyncReadServerStdout(void* pVoid)
{
	auto* window = static_cast<ServerLauncherWindow*>(pVoid);

	constexpr size_t BUFSIZE = 4096;
	DWORD dwRead, dwWritten;
	char chBuf[BUFSIZE];
	bool bSuccess = false;

	// HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	for (;;)
	{
		bSuccess = ReadFile(window->_hChildStdOut_Rd, chBuf, BUFSIZE, &dwRead, NULL);
		if (!bSuccess || dwRead == 0) continue;

		// Thread-lock block copy buffer
		{
			std::lock_guard<std::mutex> lock(_threadMutex);
			window->_asyncReadQueue.insert(window->_asyncReadQueue.end(), &chBuf[0], &chBuf[dwRead]);
		}

		// TODO: Remove this write
		// bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
		if (!bSuccess) break;
	}

	return 0;
}
#endif
