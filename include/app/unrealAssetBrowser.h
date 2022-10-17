#pragma once

#include "UEViewer/Unreal/UnCore.h"
#include "taskflow/core/executor.hpp"

class UnrealAssetBrowser
{
  public:
	UnrealAssetBrowser() = default;
	~UnrealAssetBrowser() = default;

	void LoadAssetInfos(const char* searchDir);
	void PrintRaceAssets();

	tf::Future<void> LoadAssetHandle;
  private:
	tf::Executor _taskExecutor;
	tf::Taskflow _taskFlow;
	TArray<const struct CGameFileInfo*> _packages;
};