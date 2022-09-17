// EXPLICITLY NOT USING '#pragma once' HERE!
// WE WANT THIS TO BE COMPILED BY MULTIPLE UNITS

#include <map>
#include <vector>

#include <RVCore/rttid.h>

namespace rv
{
	/*
	A collection of inline registered instances:
	This keeps track of all queried class types and ensures there is an unique
	instance of each type inside the collection at runtime. Each of the queried
	instance classes must be child classes of the collection's template class.
	BEWARE: IF YOU STOP USING A CLASS (QUERYING FOR IT WITH 'GetInstance') IT
	WILL NOT BE AVAILABLE IN THE INSTANCE LIST!
	*/
	template <class TCollectionClass>
	class InlineCollection
	{
	  private:
		// Using Directives
		using CreateInstanceFnc = TCollectionClass* (*)();
		using FactoryReg = std::map<size_t, CreateInstanceFnc>;
		using FactoryRegIt = typename FactoryReg::iterator;

		/* Static Inline Instance Block */
		static inline size_t instanceIdSeq = 0;
		static inline FactoryReg factoryReg;
		template <class T>
		static const inline size_t instanceId = instanceIdSeq++;
		template <class T>
		static const inline CreateInstanceFnc createInstanceFnc =
		    []() { return static_cast<TCollectionClass*>(new T()); };
		template <class T>
		static const inline FactoryRegIt
		    colFactoryIt = factoryReg.emplace(instanceId<T>, createInstanceFnc<T>).first;
		/* Static Inline Instance Block */

		// Runtime instances of inline registered values
		std::map<size_t, TCollectionClass*> instances;

	  public:
		InlineCollection()
		{
			// Create collection instances from factory registry
			for (auto factoryIt : factoryReg)
			{
				TCollectionClass* instance = factoryIt.second();
				instances.emplace(factoryIt.first, instance);
			}
		}

		~InlineCollection()
		{
			// Delete collection runtime instances
			for (auto instanceKV : instances)
			{
				delete instanceKV.second;
			}
			instances.clear();
		}

		template <class T>
		T* const GetInstance()
		{
			auto instanceIt = instances.find(instanceId<T>);
			if (instanceIt != instances.end())
			{
				return static_cast<T*>(instanceIt->second);
			}

			// This code actually never runs
			TCollectionClass* instance = colFactoryIt<T>->second();
			instances.emplace(instanceId<T>, instance);
			return static_cast<T*>(instance);
		}

		std::vector<TCollectionClass*> GetAllInstances()
		{
			std::vector<TCollectionClass*> instancesArray;
			for (auto instanceKV : instances)
			{
				instancesArray.push_back(instanceKV.second);
			}
			return instancesArray;
		}

		const static size_t GetStaticInstancesCount() { return instanceIdSeq; }
		size_t GetInstancesCount() { return instances.size(); }
	};
} // namespace rv
