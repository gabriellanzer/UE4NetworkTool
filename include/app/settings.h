#pragma once

// StdLib Includes
#include <string>
#include <map>

struct ImGuiContext;
struct ImGuiTextBuffer;
struct ImGuiSettingsHandler;

class Settings
{
	// Using directives
	template <typename TKey, typename TValue>
	using map = std::map<TKey, TValue>;
	using string = std::string;

  public:
	static void Init();
	static void Deinit();
	static string* Register(const string& entry, const string& value = "");

  private:
	static map<string, string*> _settingsMap;

	static void ClearAllHandler(ImGuiContext* ctx, ImGuiSettingsHandler*);
	static void* ReadOpenHandler(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
	static void ReadLineHandler(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line);
	static void ApplyAllHandler(ImGuiContext* ctx, ImGuiSettingsHandler*);
	static void WriteAllHandler(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
};