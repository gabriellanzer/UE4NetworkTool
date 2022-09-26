#pragma once

class UnrealAssetBrowser
{
  public:
	UnrealAssetBrowser() = default;
	~UnrealAssetBrowser() = default;

	void PrintAssetInfos(const char* searchDir);
};