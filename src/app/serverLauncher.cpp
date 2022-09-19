#include <app/serverLauncher.h>

// Third Party Includes
#include <fmt/core.h>
#include <imgui/imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui_internal.h>
#undef IMGUI_DEFINE_MATH_OPERATORS
#include <pfd.h>

// Internal Includes
#include <RVCore/utils.h>

ServerLauncherWindow::~ServerLauncherWindow()
{
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

	ImGui::SetNextWindowDockID(dockSpaceId, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Server Launcher", &ShouldShow))
	{
		ImGui::TextUnformatted("Launch Parameters");

		// Setup double-columns layout
		if (ImGui::BeginTable("Launch Parameters", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextColumn();
			ImGui::AddSettingsHandler();

			// Draw parameters list
			static char paramBuf[512];
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
							memcpy(paramBuf, paramEntry.c_str(), paramEntry.size() + 1);
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
							memcpy(paramBuf, paramEntry.c_str(), paramEntry.size() + 1);
							_scrollTargetParamId = _selectedParamId = i;
							isSelected = true;
						}

						// Manual highlight of selectable when hovering InputText
						const ImU32 col = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
						ImGui::GetWindowDrawList()->AddRectFilled(selMin, selMax, col);
					}

					// Selected InputText write to the paramBuf
					if (isSelected)
					{
						ImGui::SetNextItemWidth(inputTextWidth);
						if (ImGui::InputText("##paramInput", paramBuf, 512))
						{
							paramEntry = paramBuf;
						}
					}
					else // Unselected are readonly of the stored entry values
					{
						ImGui::SetNextItemWidth(inputTextWidth);
						if (ImGui::InputText("##paramInput", const_cast<char*>(paramEntry.c_str()),
											 paramEntry.size() + 1, ImGuiInputTextFlags_ReadOnly))
						{
							paramEntry = paramBuf;
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
			ImGui::TextWrapped("Info: The parameters are all appended in order and with '?' character between them (no spaces).");
			if (ImGui::Button("New Param"))
			{
				if (!_launchParams.empty())
				{
					_launchParams[_selectedParamId] = paramBuf;
					paramBuf[0] = '\0';
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
					memcpy(paramBuf, paramEntry.c_str(), paramEntry.size() + 1);
					_scrollTargetParamId = _selectedParamId;
				}
			}
			ImGui::EndDisabled();
			ImGui::EndTable();
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
			ForceCloseServer();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button("Clear Logs"))
		{
			_serverLogs.clear();
		}
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

#if WIN32
void ServerLauncherWindow::LaunchServerProcess()
{
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

	char appPathArr[_MAX_PATH + 1];
	GetModuleFileName(nullptr, appPathArr, _MAX_PATH);
	string appPath = appPathArr;
	string fileName, fileExt;
	string directory = rv::splitFilename(appPath, fileName, fileExt);
	string cmdLine = std::getenv("UE_EDITOR_PATH");
	// cmdLine += " " + directory;
	cmdLine += " D:\\Aquiris\\wc2\\HorizonChase2.uproject";
	cmdLine += " /Game/HorizonChase2/Maps/Tracks/USA/USA_CA_SanFrancisco/L_USA_CA_SanFrancisco"
			   "?countdown=15?MaxPlayers=8";
	cmdLine += "?race=/Game/HorizonChase2/Maps/Tracks/USA/USA_CA_SanFrancisco/"
			   "DA_USA_CA_SanFrancisco_Race.DA_USA_CA_SanFrancisco_Race";
	cmdLine += " -server -stdout";

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
	_hAsyncReadServerHandle = CreateThread(0, 0, AsyncReadServerStdout, this, 0, NULL);

	//    pfd::message launchSuccessDialog("Server Launch Succeeded", "Server was successfully initialized!",
	//    pfd::choice::ok,
	//				     pfd::icon::info);

	// Close handles to the stdout pipe no longer needed by the child process.
	// We have to close the stdout write handle in order to start reading from the other end.
	CloseHandle(_hChildStdOut_Wr);
}

void ServerLauncherWindow::ForceCloseServer()
{
	// Terminate async read thread
	TerminateThread(_hAsyncReadServerHandle, 0);
	CloseHandle(_hAsyncReadServerHandle);

	// Terminate server thread
	TerminateProcess(_serverProcInfo->hProcess, 0);
	CloseHandle(_serverProcInfo->hProcess);
	CloseHandle(_serverProcInfo->hThread);

	// Invalidate process info
	delete _serverProcInfo;
	_serverProcInfo = nullptr;
}

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
		_serverLogs.push_back(output);
		output.clear();
	}

	ImGui::BeginChild("Server Output Log");
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
	for (const string& logEntry : _serverLogs)
	{
		ImGui::TextUnformatted(logEntry.c_str());
	}
	ImGui::PopStyleVar();
	ImGui::EndChild();
}

std::mutex ServerLauncherWindow::_threadMutex;

DWORD ServerLauncherWindow::AsyncReadServerStdout(void* pVoid)
{
	auto* window = reinterpret_cast<ServerLauncherWindow*>(pVoid);

	constexpr size_t BUFSIZE = 4096;
	DWORD dwRead, dwWritten;
	char chBuf[BUFSIZE];
	bool bSuccess = false;

	HANDLE hParentStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

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
		bSuccess = WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
		if (!bSuccess) break;
	}

	return 0;
}
#endif
