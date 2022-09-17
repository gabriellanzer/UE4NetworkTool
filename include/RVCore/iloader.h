#ifndef __ILOADER__H__
#define __ILOADER__H__

#include <string>
#include <vector>

#include <CppDelegate/Delegate.h>

#include <RVCore/resource.h>
#include <RVCore/rttid.h>
#include <RVCore/utils.h>

#define RV_CREATE_LOADER_CMD(LOADER_TYPE, LOAD_FUNCTION, UNLOAD_FUNCTION)                                              \
	{                                                                                                              \
		SA::delegate<uint32_t(rv::Resource)>::create<LOADER_TYPE, &LOADER_TYPE::LOAD_FUNCTION>(this),          \
		    SA::delegate<void(rv::Resource)>::create<LOADER_TYPE, &LOADER_TYPE::UNLOAD_FUNCTION>(this)         \
	}

namespace rv
{
	using namespace SA;
	struct LoaderCommand
	{
		delegate<uint32_t(rv::Resource)> loadFunc;
		delegate<void(rv::Resource)> unloadFunc;
	};

	class ILoader
	{
	  public:
		ILoader() = delete;
		template <size_t IdSize>
		constexpr ILoader(const char (&loaderIdentifier)[IdSize])
		    : loaderIdentifier(crc16(loaderIdentifier, IdSize)), version(0){};

		uint32_t GetVersionedIdentifier() { return ((loaderIdentifier << 16) | version); };
		uint32_t GetIdentifier() { return (loaderIdentifier << 16); };

		uint32_t LoadResource(Resource rsc)
		{
			rsc->loadingPhase = LoadingPhase::Loading;
			return loaderCommands[version - 1].loadFunc(rsc);
		};

		uint32_t LoadResource(Resource rsc, uint32_t loaderIdentifier)
		{
			rsc->loadingPhase = LoadingPhase::Loading;
			uint16_t version = loaderIdentifier & 0xffff;
			return loaderCommands[version - 1].loadFunc(rsc);
		};

		void UnloadResource(Resource rsc)
		{
			loaderCommands[version - 1].unloadFunc(rsc);
			rsc->loadingPhase = LoadingPhase::Unloaded;
		};

		void UnloadResource(Resource rsc, uint32_t loaderIdentifier)
		{
			uint16_t version = loaderIdentifier & 0xffff;
			loaderCommands[version - 1].unloadFunc(rsc);
			rsc->loadingPhase = LoadingPhase::Unloaded;
		};

		virtual std::string GetLoadingSubdirectory() { return ""; };

	  private:
		const uint16_t loaderIdentifier;
		uint16_t version;
		std::vector<LoaderCommand> loaderCommands;

	  protected:
		inline void RegisterLoaderCmd(LoaderCommand loaderCmd)
		{
			loaderCommands.push_back(loaderCmd);
			version = loaderCommands.size();
		}
	};
} // namespace rv

#endif //!__ILOADER__H__