#include "pch.h"
#include "ECSDebugPanel.h"

#include "Engine/ImGui/ImGui.h"

namespace Hazel {

	ECSDebugPanel::ECSDebugPanel(Ref<Scene> context)
		: m_Context(context)
	{
	}

	ECSDebugPanel::~ECSDebugPanel()
	{
	}

	void ECSDebugPanel::OnImGuiRender(bool& open)
	{
		if (ImGui::Begin("ECS Debug", &open) && m_Context)
		{
			for (auto e : m_Context->m_Registry.view<IDComponent>())
			{
				Entity entity = { e, m_Context.Raw() };
				const auto& name = entity.Name();

				std::string label = fmt::format("{0} - {1}", name, entity.GetUUID());

				bool selected = SelectionManager::IsSelected(SelectionContext::Scene, entity.GetUUID());
				if (ImGui::Selectable(label.c_str(), &selected, 0))
					SelectionManager::Select(SelectionContext::Scene, entity.GetUUID());
			}

		}

		ImGui::End();
	}

}
