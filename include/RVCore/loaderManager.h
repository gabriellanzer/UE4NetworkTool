#ifndef __RV_LOADERMANAGER__H__
#define __RV_LOADERMANAGER__H__

#include <map>
#include <vector>

#include <RVCore/iloader.h>
#include <RVCore/inlineCollection.h>
#include <RVCore/resource.h>

namespace rv
{
	class LoaderManager
	{
		using CreateLoaderFnc = ILoader* (*)();
		template <class TKey, class TValue>
		using map = std::map<TKey, TValue>;
		template <class TValue>
		using vector = std::vector<TValue>;

	  public:
		LoaderManager()
		{
			// Populate loader identifier mapping
			auto instances = loaders.GetAllInstances();
			for (ILoader* loader : instances)
			{
				loaderPerIds.emplace(loader->GetIdentifier(), loader);
			}
		}

		template <class T>
		T* const GetLoader()
		{
			return loaders.GetInstance<T>();
		}

		ILoader* const GetLoaderFromId(uint32_t loaderId)
		{
			loaderId &= (0xffff << 16);
			auto it = loaderPerIds.find(loaderId);
			if (it != loaderPerIds.end())
			{
				return it->second;
			}
			return nullptr;
		}

		vector<ILoader*> GetAllLoaders() { return loaders.GetAllInstances(); }

	  private:
		InlineCollection<ILoader> loaders;
		map<uint32_t, ILoader*> loaderPerIds;
	};

} // namespace rv

#endif //!__RV_LOADERMANAGER__H__