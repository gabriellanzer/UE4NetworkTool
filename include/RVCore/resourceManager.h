#ifndef __RV_RESOURCEMANAGER__H__
#define __RV_RESOURCEMANAGER__H__

#include <map>
#include <string>
#include <vector>
#include <map>

#include <fmt/core.h>
#include <fmt/format.h>

#include <CppDelegate/MultiCastDelegate.h>

#include <RVCore/utils.h>
#include <RVCore/resource.h>
#include <RVCore/rttid.h>

namespace rv
{
	class ResourcesManager
	{
		using string = std::string;
		template <class T>
		using vector = std::vector<T>;
		template <class TKey, class TValue>
		using map = std::map<TKey, TValue>;
		template <class TKey, class TValue>
		using multimap = std::multimap<TKey, TValue>;
		template <typename TArg>
		using multicast_delegate = SA::multicast_delegate<TArg>;
		template <typename TArg>
		using delegate = SA::delegate<TArg>;

		friend class ResourceLoader;

	  public:
		ResourcesManager() = default;
		~ResourcesManager() = default;

		bool RegisterResource(Resource rsc);
		bool UnregisterResource(uint32_t rId);
		bool TryGetResource(uint32_t rId, Resource& outRsc);
		void Clear();

		template <class TData>
		inline vector<Resource> QueryResourcesOfDataType()
		{
			return QueryResourcesOfDataType(DataType<TData>);
		};
		vector<Resource> QueryResourcesOfDataType(uint32_t dataType);

		string Encode();
		void Decode(vector<string> encodedTokens, uint32_t& tokenOffset);

		template <class TData>
		inline void SubscribeOnResourceLoad(delegate<void(Resource)> callback)
		{
			auto it = dataLoadEvents.find(DataType<TData>);
			if (it == dataLoadEvents.end())
			{
				it = dataLoadEvents.emplace(DataType<TData>, new multicast_delegate<void(Resource)>()).first;
			}
			auto* del = it->second;
			*del += callback;
		};

		template <class TData, class TClass>
		inline void SubscribeOnResourceLoad(TClass* instance, void(TClass::*member)(Resource))
		{
			auto callback = delegate<void(Resource)>::create(instance, member);
			SubscribeOnResourceLoad<TData>(callback);
		};

		void SetProjectDir(string* projectDirectory);

	  private:
		ResourcesManager(ResourcesManager&&) = delete;
		ResourcesManager(const ResourcesManager&) = delete;
		ResourcesManager& operator=(ResourcesManager&&) = delete;
		ResourcesManager& operator=(const ResourcesManager&) = delete;

		void InvokeResourceDataLoaded(uint32_t dataType, Resource rsc);
		string* projectDirectory = nullptr;
		map<uint32_t, Resource> registry;
		multimap<uint32_t, Resource> dataTypeToRsc;
		map<uint32_t, multicast_delegate<void(Resource)>*> dataLoadEvents;
	};

	inline bool ResourcesManager::RegisterResource(Resource rsc)
	{
		const uint32_t rId = crc32(rsc->localPath);
		return registry.emplace(rId, rsc).second;
	}

	inline bool ResourcesManager::TryGetResource(uint32_t rId, Resource& outRsc)
	{
		auto entry = registry.find(rId);
		if (entry == registry.end())
		{
			return false;
		}
		outRsc = entry->second;
		return true;
	}

	inline bool ResourcesManager::UnregisterResource(uint32_t rId)
	{
		auto it = registry.find(rId);
		if (it != registry.end())
		{
			// TODO: Delete Data with unload callback
			for (auto dIt = dataTypeToRsc.begin(); dIt != dataTypeToRsc.end(); dIt++)
			{
				if (dIt->second == it->second)
				{
					dataTypeToRsc.erase(dIt);
					break;
				}
			}
			delete it->second;
			registry.erase(it);
			return true;
		}
		return false;
	}

	inline std::string ResourcesManager::Encode()
	{
		std::string encodedStr;
		for (const auto entry : registry)
		{
			const Resource& rsc = entry.second;
			encodedStr += fmt::format("RSC|{}|{}|{}|{}|{}", rsc->guid, rsc->localPath.c_str(),
						  rsc->remotePath.c_str(), rsc->loaderId, rsc->dependencies.size())
					  .c_str();
			for (uint32_t dep : rsc->dependencies)
			{
				encodedStr += fmt::format("|{}", dep).c_str();
			}
			encodedStr += "\n";
		}
		return encodedStr;
	}

	inline void ResourcesManager::Decode(vector<string> encodedTokens, uint32_t& tokenOffset)
	{
		while (tokenOffset < encodedTokens.size())
		{
			const int test = encodedTokens.size();
			const string& rscStr = encodedTokens[tokenOffset];
			const vector<string>& rscFields = splitStr(rscStr, "|");
			int it = 0;
			if (rscFields[it++] != "RSC")
			{
				// No more Resources to Decode!
				return;
			}
			tokenOffset++;
			Resource rsc = new Resource_T();
			rsc->guid = std::stoul(rscFields[it++].c_str());	  // Guid
			rsc->localPath = rscFields[it++];			  // Local path
			rsc->remotePath = rscFields[it++];			  // Remote path
			rsc->loaderId = std::stoul(rscFields[it++].c_str());	  // Loader ID
			size_t rscDepCount = std::stoul(rscFields[it++].c_str()); // Dependencies Count
			rsc->dependencies = std::unordered_set<uint32_t>();	  // Create Dep Hash-Set
			for (size_t i = 0; i < rscDepCount; i++)		  // Insert Entries
			{
				rsc->dependencies.emplace(atoi(rscFields[it + i].c_str())); // Dependency Guid
			}
			rsc->projectDir = projectDirectory;
			registry.emplace(rsc->guid, rsc);
		}
	}

	inline void ResourcesManager::Clear()
	{
		for (auto it = dataLoadEvents.begin(); it != dataLoadEvents.end(); it++)
		{
			delete it->second;
		}
		dataLoadEvents.clear();
		for (auto it = registry.begin(); it != registry.end(); it++)
		{
			// TODO: Delete Data with unload callback
			delete it->second;
		}
		registry.clear();
		dataTypeToRsc.clear();
	}

	inline void ResourcesManager::InvokeResourceDataLoaded(uint32_t dataType, Resource rsc)
	{
		// Map Data Type to Resource instance
		dataTypeToRsc.emplace(dataType, rsc);
		// Invoke Data Load Event
		auto it = dataLoadEvents.find(dataType);
		if (it != dataLoadEvents.end())
		{
			(*it->second)(rsc);
		}
	}

	inline std::vector<Resource> ResourcesManager::QueryResourcesOfDataType(uint32_t dataType)
	{
		vector<Resource> queryRsc;
		for (auto it = dataTypeToRsc.find(dataType); it != dataTypeToRsc.end(); it++)
		{
			if (it->first != dataType)
			{
				break;
			}
			queryRsc.push_back(it->second);
		}
		return queryRsc;
	}

	inline void ResourcesManager::SetProjectDir(string* projectDirectory)
	{
		this->projectDirectory = projectDirectory;
		for (auto rscEntry : registry)
		{
			Resource rsc = rscEntry.second;
			rsc->projectDir = projectDirectory;
		}
	}

} // namespace rv

#endif //!__RV_RESOURCEMANAGER__H__