#pragma once

#include "Hazel/Renderer/SceneRenderer.h"
#include "Hazel/Renderer/RenderCommandBuffer.h"
#include "Hazel/Scene/Scene.h"
#include "Hazel/Scene/Entity.h"

namespace Hazel {

	struct ThumbnailImage
	{
		Ref<Image2D> Image;
		uint64_t LastWriteTime;

		operator bool() const { return (bool)Image; }
	};

	class AssetThumbnailGenerator : public RefCounted
	{
	public:
		// Setup Scene and SceneRenderer as required
		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) = 0;
		// Undo all Scene/SceneRenderer changes
		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> sceneRenderer) = 0;
	};

	class ThumbnailGenerator : public RefCounted
	{
	public:
		ThumbnailGenerator();
		~ThumbnailGenerator();

		Ref<Image2D> GenerateThumbnail(AssetHandle handle);
	private:
		Ref<Scene> m_Scene, m_EnvironmentScene;
		Ref<SceneRenderer> m_SceneRenderer;
		Ref<RenderCommandBuffer> m_RenderCommandBuffer;

		Entity m_CameraEntity;
		glm::vec3 m_CameraForward;

		uint32_t m_Width = 256, m_Height = 256;

		std::unordered_map<AssetType, Ref<AssetThumbnailGenerator>> m_AssetThumbnailGenerators;
	};

}
