#include "hzpch.h"
#include "SelectionManager.h"
#include "Engine/Core/Application.h"
#include "Engine/Core/Events/SceneEvents.h"

namespace Hazel {

	void SelectionManager::Select(SelectionContext contextID, UUID selectionID)
	{
		auto& contextSelections = s_Contexts[contextID];
		if (std::find(contextSelections.begin(), contextSelections.end(), selectionID) != contextSelections.end())
			return;

		// TODO: Maybe verify if the selectionID is already selected in another context?

		contextSelections.push_back(selectionID);
		Application::Get().DispatchEvent<SelectionChangedEvent>(contextID, selectionID, true);
	}
	
	bool SelectionManager::IsSelected(UUID selectionID)
	{
		for (const auto& [contextID, contextSelections] : s_Contexts)
		{
			if (std::find(contextSelections.begin(), contextSelections.end(), selectionID) != contextSelections.end())
			{
				return true;
			}
		}

		return false;
	}
	
	bool SelectionManager::IsSelected(SelectionContext contextID, UUID selectionID)
	{
		const auto& contextSelections = s_Contexts[contextID];
		return std::find(contextSelections.begin(), contextSelections.end(), selectionID) != contextSelections.end();
	}
	
	bool SelectionManager::IsEntityOrAncestorSelected(const Entity entity)
	{
		Entity e = entity;
		while (e)
		{
			if (IsSelected(e.GetUUID()))
			{
				return true;
			}
			e = e.GetParent();
		}
		return false;
	}

	bool SelectionManager::IsEntityOrAncestorSelected(SelectionContext contextID, const Entity entity)
	{
		Entity e = entity;
		while (e)
		{
			if (IsSelected(contextID, e.GetUUID()))
			{
				return true;
			}
			e = e.GetParent();
		}
		return false;
	}

	void SelectionManager::Deselect(UUID selectionID)
	{
		for (auto& [contextID, contextSelections] : s_Contexts)
		{
			auto it = std::find(contextSelections.begin(), contextSelections.end(), selectionID);
			if (it == contextSelections.end())
				continue;

			Application::Get().DispatchEvent<SelectionChangedEvent>(contextID, selectionID, false);
			contextSelections.erase(it);
			break;
		}
	}
	
	void SelectionManager::Deselect(SelectionContext contextID, UUID selectionID)
	{
		auto& contextSelections = s_Contexts[contextID];
		auto it = std::find(contextSelections.begin(), contextSelections.end(), selectionID);
		if (it == contextSelections.end())
			return;

		contextSelections.erase(it);
	}

	void SelectionManager::DeselectAll()
	{
		for (auto& [ctxID, contextSelections] : s_Contexts)
		{
			for (const auto& selectionID : contextSelections)
				Application::Get().DispatchEvent<SelectionChangedEvent>(ctxID, selectionID, false);
			contextSelections.clear();
		}
	}

	void SelectionManager::DeselectAll(SelectionContext contextID)
	{
		auto& contextSelections = s_Contexts[contextID];

		for (const auto& selectionID : contextSelections)
			Application::Get().DispatchEvent<SelectionChangedEvent>(contextID, selectionID, false);

		contextSelections.clear();
	}

	UUID SelectionManager::GetSelection(SelectionContext context, size_t index)
	{
		auto& contextSelections = s_Contexts[context];
		HZ_CORE_VERIFY(index >= 0 && index < contextSelections.size());
		return contextSelections[index];
	}

	size_t SelectionManager::GetSelectionCount(SelectionContext contextID)
	{
		return s_Contexts[contextID].size();
	}

}
