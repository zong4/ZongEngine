#include <pch.h>
#include "AudioObject.h"

namespace Hazel
{
#if 0
	AudioObject::AudioObject(UUID ID) : m_ID(ID) {}
	AudioObject::AudioObject(UUID ID, const std::string& debugName) : m_ID(ID), m_DebugName(debugName) {}

	AudioObject::AudioObject(const AudioObject&& other) noexcept
		: m_ID(other.m_ID)
		, m_Transform(other.m_Transform)
		, m_DebugName(other.m_DebugName)
		, m_Released(other.m_Released)
	{
	}

	AudioObject& AudioObject::operator=(const AudioObject&& other) noexcept
	{
		m_ID = other.m_ID;
		m_Transform = other.m_Transform;
		m_DebugName = other.m_DebugName;
		m_Released = other.m_Released;
		return *this;
	};

	void AudioObject::SetTransform(const Audio::Transform& transform)
	{
		m_Transform = transform;
	}

	void AudioObject::SetVelocity(const glm::vec3& velocity)
	{
		m_Velocity = velocity;
	}
#endif

} // namespace Hazel
