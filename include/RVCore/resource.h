#ifndef __RV_RESOURCE__H__
#define __RV_RESOURCE__H__

#include <CppDelegate/Delegate.h>
#include <unordered_set>
#include <string>
#include <RVCore/utils.h>

namespace rv
{
	/**
	 * @brief The resource loading phase enum.
	 */
	enum class LoadingPhase : uint16_t
	{
		/**
		 * @brief Newly created resource (no loading).
		 */
		New,
		/**
		 * @brief Resource on the download queue.
		 */
		DownloadEnqueued,
		/**
		 * @brief Resource being downloaded.
		 */
		Downloading,
		/**
		 * @brief Resource on the load queue.
		 */
		LoadEnqueue,
		/**
		 * @brief Resource being loaded.
		 */
		Loading,
		/**
		 * @brief Resource is stalled waiting for dependencies.
		 */
		Stalled,
		/**
		 * @brief Resource has been fully loaded, with it's dependencies.
		 */
		Complete,
		/**
		 * @brief Resource has been unloaded.
		 */
		Unloaded,
		/**
		 * @brief Resource is in error state.
		 */
		Error
	};

	/**
	 * @brief Abstract representation of a Serialized Resource.
	 */
	struct Resource_T
	{
		/**
		 * @brief Hash value of the relative resource path (to the project folder).
		 */
		uint32_t guid;

		/**
		 * @brief Identifier for this resource loader.
		 */
		uint32_t loaderId;

		/**
		 * @brief Current resource loading phase.
		 */
		LoadingPhase loadingPhase;

		/**
		 * @brief Pointer to the Resource data.
		 */
		void* data;

		/**
		 * @brief Local resource path (relative to project folder).
		 */
		std::string localPath;

		/**
		 * @brief Remote resource path (source path or URI), might be empty if local-only.
		 */
		std::string remotePath;

		/**
		 * @brief Set of all ids whose resources this one depends upon.
		 */
		std::unordered_set<uint32_t> dependencies;

		/**
		 * @brief Delegate that returns the resource full path.
		 */
		std::string getFullPath() {
			return *projectDir + localPath;
		};

		std::string* projectDir;
	};

	typedef Resource_T* Resource;

} // namespace rv

#endif //!__RV_RESOURCE__H__