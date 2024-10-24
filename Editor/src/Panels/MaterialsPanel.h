#pragma once

#include "Engine/Editor/EditorPanel.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Renderer/Texture.h"

namespace Engine {

	class MaterialsPanel : public EditorPanel
	{
	public:
		MaterialsPanel();
		~MaterialsPanel();

		virtual void SetSceneContext(const Ref<Scene>& context) override;
		virtual void OnImGuiRender(bool& isOpen) override;

	private:
		void RenderMaterial(size_t materialIndex, AssetHandle materialAssetHandle);

	private:
		Ref<Scene> m_Context;
		Entity m_SelectedEntity;
		Ref<Texture2D> m_CheckerBoardTexture;
	};

}
