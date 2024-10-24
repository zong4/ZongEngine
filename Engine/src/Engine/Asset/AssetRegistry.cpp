#include "pch.h"
#include "AssetRegistry.h"

namespace Engine {

#define ZONG_ASSETREGISTRY_LOG 0
#if ZONG_ASSETREGISTRY_LOG
#define ASSET_LOG(...) ZONG_CORE_TRACE_TAG("ASSET", __VA_ARGS__)
#else 
#define ASSET_LOG(...)
#endif

	static std::mutex s_AssetRegistryMutex;

	AssetMetadata& AssetRegistry::operator[](const AssetHandle handle)
	{
		std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);

		ASSET_LOG("Retrieving handle {}", handle);
		return m_AssetRegistry[handle];
	}

	const AssetMetadata& AssetRegistry::Get(const AssetHandle handle) const
	{
		std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);

		ZONG_CORE_ASSERT(m_AssetRegistry.find(handle) != m_AssetRegistry.end());
		ASSET_LOG("Retrieving const handle {}", handle);
		return m_AssetRegistry.at(handle);
	}

	AssetMetadata& AssetRegistry::Get(const AssetHandle handle)
	{
		std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);

		ASSET_LOG("Retrieving handle {}", handle);
		return m_AssetRegistry[handle];
	}

	bool AssetRegistry::Contains(const AssetHandle handle) const
	{
		std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);

		ASSET_LOG("Contains handle {}", handle);
		return m_AssetRegistry.find(handle) != m_AssetRegistry.end();
	}

	size_t AssetRegistry::Remove(const AssetHandle handle)
	{
		std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);

		ASSET_LOG("Removing handle", handle);
		return m_AssetRegistry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		std::scoped_lock<std::mutex> lock(s_AssetRegistryMutex);

		ASSET_LOG("Clearing registry");
		m_AssetRegistry.clear();
	}

}
