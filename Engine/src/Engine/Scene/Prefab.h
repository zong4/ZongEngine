#pragma once

#include "Engine/Asset/Asset.h"

#include "Engine/Scene/Scene.h"
#include "entt/entt.hpp"

namespace Engine {

	class Prefab : public Asset
	{
	public:
		// Create prefab with empty entity
		Prefab();
		~Prefab();

		// Replaces existing entity if present
		void Create(Entity entity, bool serialize = true);

		static AssetType GetStaticType() { return AssetType::Prefab; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		std::unordered_set<AssetHandle> GetAssetList(bool recursive = true);
	private:
		Entity CreatePrefabFromEntity(Entity entity);

	private:
		Ref<Scene> m_Scene;
		Entity m_Entity;

		friend class Scene;
		friend class PrefabEditor;
		friend class PrefabSerializer;
		friend class ScriptEngine;
	};

}
