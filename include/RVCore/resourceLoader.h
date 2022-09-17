#ifndef __RESOURCELOADER__H__
#define __RESOURCELOADER__H__

#include <string>
#include <fstream>

#include <RVCore/iloader.h>
#include <RVCore/loaderManager.h>
#include <RVCore/resource.h>
#include <RVCore/resourceManager.h>
#include <RVCore/utils.h>

namespace rv
{	
	class ResourceLoader
	{
		using string = std::string;
		template <class T>
		using unordered_set = std::unordered_set<T>;

	  public:
		constexpr ResourceLoader(LoaderManager* loaderManager, ResourcesManager* rscManager)
		    : projectDir(nullptr), loaderManager(loaderManager), rscManager(rscManager)
		{
		}

		template <class TLoader>
		Resource LoadResourceFromPath(string fileLoadPath)
		{
			ILoader* const loader = loaderManager->GetLoader<TLoader>();
			string subDir = loader->GetLoadingSubdirectory();

			string fileName;
			splitFilename(fileLoadPath, fileName);

			string localPath = subDir + fileName;

			Resource rsc;
			uint32_t guid = crc32(localPath);
			if (rscManager->TryGetResource(guid, rsc))
			{
				return rsc;
			}

			string fullPath = *projectDir + localPath;
			if (!fileExists(fullPath))
			{
				copyFile(fileLoadPath, fullPath);
			}

			rsc = new Resource_T();
			rsc->localPath = localPath;
			rsc->guid = crc32(localPath);
			rsc->remotePath = fileLoadPath;
			rsc->loaderId = loader->GetVersionedIdentifier();
			rsc->dependencies = unordered_set<uint32_t>();
			rsc->loadingPhase = LoadingPhase::LoadEnqueue;
			rsc->projectDir = projectDir;
			rscManager->RegisterResource(rsc);

			// Invoke latest version of the Load function
			uint32_t dataType = loader->LoadResource(rsc);
			rscManager->InvokeResourceDataLoaded(dataType, rsc);
			return rsc;
		}

		void LoadAllResources()
		{
			for (auto entry : rscManager->registry)
			{
				Resource rsc = entry.second;
				ILoader* loader = loaderManager->GetLoaderFromId(rsc->loaderId);
				uint32_t dataType = loader->LoadResource(rsc, rsc->loaderId);
				rscManager->InvokeResourceDataLoaded(dataType, rsc);
			}
		}

		void SetProjectDir(string* projectDir) { this->projectDir = projectDir; }

	  private:
		string* projectDir;
		ResourcesManager* rscManager;
		LoaderManager* loaderManager;
	};
} // namespace rv

#endif //!__RESOURCELOADER__H__