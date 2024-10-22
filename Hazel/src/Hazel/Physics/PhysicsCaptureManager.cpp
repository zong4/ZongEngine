#include "hzpch.h"
#include "PhysicsCaptureManager.h"

#include "Hazel/Physics/PhysicsSystem.h"

#include "Hazel/Utilities/FileSystem.h"

namespace Hazel {

	void PhysicsCaptureManager::ClearCaptures()
	{
		m_Captures.clear();
		m_RecentCapture = "";
	}

	void PhysicsCaptureManager::RemoveCapture(const std::filesystem::path& capturePath)
	{
		m_Captures.erase(std::remove_if(m_Captures.begin(), m_Captures.end(), [&capturePath](const std::filesystem::path& filepath)
		{
			return filepath == capturePath;
		}));
	}

	PhysicsCaptureManager::PhysicsCaptureManager()
	{
		switch (PhysicsAPI::Current())
		{
			case PhysicsAPIType::Jolt:
				m_CapturesDirectory = std::filesystem::path("Captures") / "Jolt";
				break;
		}

		if (!FileSystem::Exists(m_CapturesDirectory))
			FileSystem::CreateDirectory(m_CapturesDirectory);

		for (auto capturePath : std::filesystem::directory_iterator(m_CapturesDirectory))
		{
			std::filesystem::path path = capturePath.path();
			m_Captures.push_back(path);

			if (m_RecentCapture.empty())
			{
				m_RecentCapture = path;
				continue;
			}

			if (FileSystem::IsNewer(path, m_RecentCapture))
				m_RecentCapture = path;
		}
	}

}
