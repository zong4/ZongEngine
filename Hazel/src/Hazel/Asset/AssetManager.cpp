#include "hzpch.h"
#include "AssetManager.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/UI/Font.h"

namespace Hazel {

	static std::unordered_map<AssetType, std::function<Ref<Asset>()>> s_AssetPlaceholderTable =
	{
		{ AssetType::Texture, []() { return Renderer::GetWhiteTexture(); }},
		{ AssetType::EnvMap, []() { return Renderer::GetEmptyEnvironment(); }},
		{ AssetType::Font, []() { return Font::GetDefaultFont(); }},
	};

	Ref<Asset> AssetManager::GetPlaceholderAsset(AssetType type)
	{
		if (s_AssetPlaceholderTable.contains(type))
			return s_AssetPlaceholderTable.at(type)();

		return nullptr;
	}
}
