#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Renderer/Material.h"

#include <map>

namespace Engine {

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset(bool transparent = false);
		MaterialAsset(Ref<Material> material);
		~MaterialAsset();

		glm::vec3& GetAlbedoColor();
		void SetAlbedoColor(const glm::vec3& color);

		float& GetMetalness();
		void SetMetalness(float value);

		float& GetRoughness();
		void SetRoughness(float value);

		float& GetEmission();
		void SetEmission(float value);

		// Textures
		Ref<Texture2D> GetAlbedoMap();
		void SetAlbedoMap(Ref<Texture2D> texture);
		void ClearAlbedoMap();

		Ref<Texture2D> GetNormalMap();
		void SetNormalMap(Ref<Texture2D> texture);
		bool IsUsingNormalMap();
		void SetUseNormalMap(bool value);
		void ClearNormalMap();

		Ref<Texture2D> GetMetalnessMap();
		void SetMetalnessMap(Ref<Texture2D> texture);
		void ClearMetalnessMap();

		Ref<Texture2D> GetRoughnessMap();
		void SetRoughnessMap(Ref<Texture2D> texture);
		void ClearRoughnessMap();

		float& GetTransparency();
		void SetTransparency(float transparency);

		bool IsShadowCasting() const { return !m_Material->GetFlag(MaterialFlag::DisableShadowCasting); }
		void SetShadowCasting(bool castsShadows) { return m_Material->SetFlag(MaterialFlag::DisableShadowCasting, !castsShadows); }

		static AssetType GetStaticType() { return AssetType::Material; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		Ref<Material> GetMaterial() const { return m_Material; }
		void SetMaterial(Ref<Material> material) { m_Material = material; }

		bool IsTransparent() const { return m_Transparent; }
	private:
		void SetDefaults();
	private:
		Ref<Material> m_Material;
		bool m_Transparent = false;

		friend class MaterialEditor;
	};

	class MaterialTable : public RefCounted
	{
	public:
		MaterialTable(uint32_t materialCount = 1);
		MaterialTable(Ref<MaterialTable> other);
		~MaterialTable() = default;

		bool HasMaterial(uint32_t materialIndex) const { return m_Materials.find(materialIndex) != m_Materials.end(); }
		void SetMaterial(uint32_t index, AssetHandle material);
		void ClearMaterial(uint32_t index);

		AssetHandle GetMaterial(uint32_t materialIndex) const
		{
			ZONG_CORE_ASSERT(HasMaterial(materialIndex));
			return m_Materials.at(materialIndex);
		}
		std::map<uint32_t, AssetHandle>& GetMaterials() { return m_Materials; }
		const std::map<uint32_t, AssetHandle>& GetMaterials() const { return m_Materials; }

		uint32_t GetMaterialCount() const { return m_MaterialCount; }
		void SetMaterialCount(uint32_t materialCount) { m_MaterialCount = materialCount; }

		void Clear();
	private:
		std::map<uint32_t, AssetHandle> m_Materials;
		uint32_t m_MaterialCount;
	};

}
