#pragma once

namespace Hazel {

	template<typename T, typename... Args>
	T& Entity::AddComponent(Args&&... args)
	{
		HZ_CORE_VERIFY(!HasComponent<T>(), "Entity already has component!");
		return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
	}

	template<typename T>
	T& Entity::GetComponent()
	{
		HZ_CORE_VERIFY(HasComponent<T>(), "Entity doesn't have component!");
		return m_Scene->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	const T& Entity::GetComponent() const
	{
		HZ_CORE_VERIFY(HasComponent<T>(), "Entity doesn't have component!");
		return m_Scene->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	T* Entity::TryGetComponent()
	{
		HZ_CORE_VERIFY(IsValid());
		return m_Scene->m_Registry.try_get<T>(m_EntityHandle);
	}

	template<typename T>
	const T* Entity::TryGetComponent() const
	{
		HZ_CORE_VERIFY(IsValid());
		return m_Scene->m_Registry.try_get<T>(m_EntityHandle);
	}

	template<typename... T>
	bool Entity::HasComponent()
	{
		HZ_CORE_VERIFY(IsValid());
		return m_Scene->m_Registry.has<T...>(m_EntityHandle);
	}

	template<typename... T>
	bool Entity::HasComponent() const
	{
		HZ_CORE_VERIFY(IsValid());
		return m_Scene->m_Registry.has<T...>(m_EntityHandle);
	}

	template<typename...T>
	bool Entity::HasAny()
	{
		HZ_CORE_VERIFY(IsValid());
		return m_Scene->m_Registry.any<T...>(m_EntityHandle);
	}

	template<typename...T>
	bool Entity::HasAny() const
	{
		HZ_CORE_VERIFY(IsValid());
		return m_Scene->m_Registry.any<T...>(m_EntityHandle);
	}

	template<typename T>
	void Entity::RemoveComponent()
	{
		HZ_CORE_ASSERT(HasComponent<T>(), "Entity doesn't have component!");
		m_Scene->m_Registry.remove<T>(m_EntityHandle);
	}

	template<typename T>
	void Entity::RemoveComponentIfExists()
	{
		HZ_CORE_VERIFY(IsValid());
		m_Scene->m_Registry.remove_if_exists<T>(m_EntityHandle);
	}

}
