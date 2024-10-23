#pragma once

#include "AssetEditorPanel.h"
#include "Engine/Renderer/Mesh.h"

#include "Engine/Scene/Prefab.h"
#include "Engine/Editor/SceneHierarchyPanel.h"

namespace Audio
{
	struct SoundConfig;
}

namespace Hazel {

	class AnimationControllerAsset;
	struct SoundConfig;

	class MaterialEditor : public AssetEditor
	{
	public:
		MaterialEditor();

		virtual void SetAsset(const Ref<Asset>& asset) override { m_MaterialAsset = (Ref<MaterialAsset>)asset; }

	private:
		virtual void OnOpen() override;
		virtual void OnClose() override;
		virtual void Render() override;

	private:
		Ref<MaterialAsset> m_MaterialAsset;
	};

	class PrefabEditor : public AssetEditor
	{
	public:
		PrefabEditor();

		virtual void SetAsset(const Ref<Asset>& asset) override { m_Prefab = (Ref<Prefab>)asset; m_SceneHierarchyPanel.SetSceneContext(m_Prefab->m_Scene); 	}

	private:
		virtual void OnOpen() override;
		virtual void OnClose() override;
		virtual void Render() override;

	private:
		Ref<Prefab> m_Prefab;
		SceneHierarchyPanel m_SceneHierarchyPanel;
	};

	class TextureViewer : public AssetEditor
	{
	public:
		TextureViewer();

		virtual void SetAsset(const Ref<Asset>& asset) override { m_Asset = (Ref<Texture>)asset; }

	private:
		virtual void OnOpen() override;
		virtual void OnClose() override;
		virtual void Render() override;

	private:
		Ref<Texture> m_Asset;
	};

	class AudioFileViewer : public AssetEditor
	{
	public:
		AudioFileViewer();

		virtual void SetAsset(const Ref<Asset>& asset) override;

	private:
		virtual void OnOpen() override;
		virtual void OnClose() override;
		virtual void Render() override;

	private:
		Ref<AudioFile> m_Asset;
	};

	class SoundConfigEditor : public AssetEditor
	{
	public:
		SoundConfigEditor();

		virtual void SetAsset(const Ref<Asset>& asset) override;

	private:
		virtual void OnOpen() override;
		virtual void OnClose() override;
		virtual void Render() override;

	private:
		Ref<SoundConfig> m_Asset;
	};

}
