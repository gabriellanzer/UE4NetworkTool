#include <app/unrealAssetBrowser.h>

// Third Party Includes
#include <UEViewer/Unreal/UnCore.h>
#include <UEViewer/Unreal/UnrealPackage/PackageUtils.h>
#include "UEViewer/Unreal/UnrealPackage/UnPackage.h"

bool GExportInProgress = false;
bool UE4EncryptedPak() { return false; }
int UE4UnversionedPackage(int verMin, int verMax)
{
	appErrorNoLog("Unversioned UE4 packages are not supported. Please restart UModel and select UE4 version in range "
				  "%d-%d using UI or command line.",
				  verMin, verMax);
	return -1;
}

void UnrealAssetBrowser::PrintAssetInfos()
{
	appSetRootDirectory("D:\\Aquiris\\wc2\\Content\\");

	TArray<const CGameFileInfo*> Packages;
	if (!Packages.Num())
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
		Packages.Empty(NumPackages);
		// Fill Packages list
		appEnumGameFiles<TArray<const CGameFileInfo*>>(
			[](const CGameFileInfo* file, TArray<const CGameFileInfo*>& param) -> bool
			{
				param.Add(file);
				return true;
			},
			Packages);

		// Perform sort - 1st part of UIPackageDialog::SortPackages()
		// SortPackages(Packages, 0, false);
	}

	ScanContent(Packages, nullptr);

	FString Path = "";
	for (const CGameFileInfo* fileInfo : Packages)
	{
		if (!fileInfo->IsPackage()) continue;

		UnPackage* package = fileInfo->Package;
		if (!package)
		{
			// Shouldn't happen, but happens (ScanPackage() should fill Package for all CGameFileInfo).
			// Example of appearance: ScanPackages may open a WRONG find in a case when game files are extracted from
			// pak, and user supplies wrong (shorter) game path, e.g. -path=Extported/Meshes, and multiple files with
			// the same name exists inside that folder. CGameFileInfo::Find has heuristic for finding files using
			// partial path, but it may fail. In this case, CGameFileInfo will say "it's package", but actually file
			// won't be scanned and/or loaded.
			// appNotify("Strange package: IsPackage=true, Package is NULL: %s (size %d Kb)", *info->GetRelativeName(),
			// info->SizeInKb + info->ExtraSizeInKb);
			continue;
		}

		for (int importIndex = 0; importIndex < package->Summary.ImportCount; importIndex++)
		{
			FObjectImport& imp = package->GetImport(importIndex);
			if (strcmp(*imp.ObjectName, "RaceData") != 0) continue;

			// Print package path using stdout
			fileInfo->GetRelativeName(Path);
			fprintf(stdout, "Package (%s)\n", *Path);
		}
	}

	fflush(stdout);
}