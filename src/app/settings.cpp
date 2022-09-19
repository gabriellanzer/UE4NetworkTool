#include <app/settings.h>

// Third Party Includes
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

// Using directives
template <typename TKey, typename TValue>
using map = std::map<TKey, TValue>;
using string = std::string;
using string_view = std::string_view;

map<string, string*> Settings::_settingsMap;

void Settings::Init()
{
	ImGuiSettingsHandler iniHandler;
	iniHandler.TypeName = "UserSettings";
	iniHandler.TypeHash = ImHashStr(iniHandler.TypeName);
	iniHandler.ClearAllFn = ClearAllHandler;
	iniHandler.ReadOpenFn = ReadOpenHandler;
	iniHandler.ReadLineFn = ReadLineHandler;
	iniHandler.ApplyAllFn = ApplyAllHandler;
	iniHandler.WriteAllFn = WriteAllHandler;
	ImGui::GetCurrentContext()->SettingsHandlers.push_back(iniHandler);
}

void Settings::Deinit()
{

	for (auto& entry : _settingsMap)
	{
		delete entry.second;
		entry.second = nullptr;
	}
}

string* Settings::Register(const string& entry, const string& value)
{
	auto [it, emplaced] = _settingsMap.try_emplace(entry, new string(value));
	return it->second;
}

void Settings::ClearAllHandler(ImGuiContext*, ImGuiSettingsHandler*) { _settingsMap.clear(); }

void* Settings::ReadOpenHandler(ImGuiContext*, ImGuiSettingsHandler*, const char*) { return &_settingsMap; }

void Settings::ReadLineHandler(ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line)
{
	string_view lineView(line);
	size_t pos = lineView.find_first_of('=');
	if (pos == size_t(-1)) return;
	string_view entry = lineView.substr(0, pos);
	string_view value = lineView.substr(pos + 1);

	if (auto it = _settingsMap.find(string(entry)); it != _settingsMap.end())
	{
		*it->second = value;
	}
}

void Settings::ApplyAllHandler(ImGuiContext*, ImGuiSettingsHandler*) {}

void Settings::WriteAllHandler(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf)
{
	buf->reserve(buf->size() + static_cast<int>(_settingsMap.size()) * 30);
	buf->appendf("[%s][%s]\n", handler->TypeName, "Global");

	for (const auto& [entry, value] : _settingsMap)
	{
		buf->appendf("%s=%s\n", entry.data(), value->data());
	}

	buf->append("\n");
}
