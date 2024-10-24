#pragma once

#include "AssetMetadata.h"

#include <unordered_map>

namespace Engine {

	class AssetRegistry
	{
	public:
		AssetMetadata& operator[](const AssetHandle handle);
		AssetMetadata& Get(const AssetHandle handle);
		const AssetMetadata& Get(const AssetHandle handle) const;

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
