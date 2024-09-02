#pragma once

#include "Assert.h"
#include "AssetMetadata.h"

#include <thread>
#include <unordered_map>

namespace Hazel {

	// WARNING: The AssetRegistry is not itself thread-safe, so if accessing AssetRegistry
	// via multiple threads, you must take care to provide your own synchronization.
	class AssetRegistry
	{
	public:

		// note: no non-const Get() function.  If you need to modify the metadata, use Set().
		// This aids correct usage in a multi-threaded environment.
		const AssetMetadata& Get(const AssetHandle handle) const;
		void Set(const AssetHandle handle, const AssetMetadata& metadata);

		size_t Count() const { return m_AssetRegistry.size(); }
		bool Contains(const AssetHandle handle) const;
		size_t Remove(const AssetHandle handle);
		void Clear();

		auto begin() { return m_AssetRegistry.begin(); }
		auto end() { return m_AssetRegistry.end(); }
		auto begin() const { return m_AssetRegistry.cbegin(); }
		auto end() const { return m_AssetRegistry.cend(); }
	private:
		std::unordered_map<AssetHandle, AssetMetadata> m_AssetRegistry;
	};

}
