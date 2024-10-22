#include "hzpch.h"
#include "Scene.h"

namespace Hazel {
	UUID Entity::GetSceneUUID() const
	{
		return m_Scene->m_SceneID;
	}

	Entity Entity::GetParent() const
	{
		return m_Scene->TryGetEntityWithUUID(GetParentUUID());
	}

	bool Entity::IsAncestorOf(Entity entity) const
	{
		const auto& children = Children();

		if (children.empty())
			return false;

		for (UUID child : children)
		{
			if (child == entity.GetUUID())
				return true;
		}

		for (UUID child : children)
		{
			if (m_Scene->GetEntityWithUUID(child).IsAncestorOf(entity))
				return true;
		}

		return false;
	}

	Entity::operator bool() const { return (m_EntityHandle != entt::null) && m_Scene && m_Scene->m_Registry.valid(m_EntityHandle); }
}
