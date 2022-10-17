#include <app/unrealAssetBrowser.h>

// Third Party Includes
#include <fmt/format.h>
#include <UEViewer/Unreal/UnCore.h>
#include <UEViewer/Unreal/UnObject.h>
#include <UEViewer/Unreal/UnrealPackage/UnPackage.h>
#include <UEViewer/Unreal/UnrealPackage/PackageUtils.h>

// Using Directives
using string = std::string;

bool GExportInProgress = false;
bool UE4EncryptedPak() { return false; }
int UE4UnversionedPackage(int verMin, int verMax)
{
	appErrorNoLog("Unversioned UE4 packages are not supported. Please restart UModel and select UE4 version in range "
				  "%d-%d using UI or command line.",
				  verMin, verMax);
	return -1;
}

void UnrealAssetBrowser::LoadAssetInfos(const char* searchDir)
{
	appSetRootDirectory(searchDir);

	_taskFlow.emplace(
		[&]()
		{
			if (!_packages.Num())
			{
				// Package list was not filled yet
				// Count packages first for more efficient Packages.Add() when number of packages is very large
				int NumPackages = 0;
				appEnumGameFiles<int&>(
					[](const CGameFileInfo* file, int& param) -> bool
					{
						param++;
						return true;
					},
					NumPackages);
				_packages.Empty(NumPackages);
				// Fill Packages list
				appEnumGameFiles<TArray<const CGameFileInfo*>>(
					[](const CGameFileInfo* file, TArray<const CGameFileInfo*>& param) -> bool
					{
						param.Add(file);
						return true;
					},
					_packages);

				// Perform sort - 1st part of UIPackageDialog::SortPackages()
				// SortPackages(Packages, 0, false);
			}

			ScanContent(_packages, nullptr);
		});

	LoadAssetHandle = _taskExecutor.run(_taskFlow);
}

void UnrealAssetBrowser::PrintRaceAssets() {
	_taskExecutor.wait_for_all();

	FString filePath = "";
	for (const CGameFileInfo* fileInfo : _packages)
	{
		if (!fileInfo->IsPackage()) continue;

		UnPackage* package = fileInfo->Package;
		if (!package)
		{
			// Shouldn't happen, but happens (ScanPackage() should fill Package for all CGameFileInfo).
			// Example of appearance: ScanPackages may open a WRONG find in a case when game files are extracted
			// from pak, and user supplies wrong (shorter) game path, e.g. -path=Extported/Meshes, and multiple
			// files with the same name exists inside that folder. CGameFileInfo::Find has heuristic for finding
			// files using partial path, but it may fail. In this case, CGameFileInfo will say "it's package",
			// but actually file won't be scanned and/or loaded. appNotify("Strange package: IsPackage=true,
			// Package is NULL: %s (size %d Kb)", *info->GetRelativeName(), info->SizeInKb +
			// info->ExtraSizeInKb);
			continue;
		}

		for (int exportIndex = 0; exportIndex < package->Summary.ExportCount; exportIndex++)
		{
			FObjectExport& exp = package->GetExport(exportIndex);
			const char* className = package->GetClassNameFor(exp);
			if (strcmp(className, "RaceData") != 0) continue;

			// Get Object Export Name
			char objFileName[128];
			objFileName[0] = '\0';
			package->GetFullExportName(exp, objFileName, sizeof(objFileName));
			// Get asset file path without extension (yet with the '.' at the end)
			fileInfo->GetRelativeName(filePath);
			string assetFilePath(*filePath, filePath.Len() - strlen(fileInfo->GetExtension()));
			// Compose final path name
			string exportPath = fmt::format("/Game/{0}{1}", assetFilePath, objFileName);
			fprintf(stdout, "%s\n", exportPath.c_str());

			LoadWholePackage(package, nullptr);
			UObject* obj = exp.Object;

		}
	}
	fflush(stdout);
}
