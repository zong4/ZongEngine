#pragma once

#include "Hazel/Editor/EditorPanel.h"
#include "Hazel/Scene/Entity.h"
#include "Hazel/Renderer/Texture.h"

namespace Hazel {

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
