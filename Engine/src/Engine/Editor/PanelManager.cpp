#include "hzpch.h"
#include "PanelManager.h"
#include "Engine/Utilities/FileSystem.h"

#include <yaml-cpp/yaml.h>

namespace Hazel {

	PanelManager::~PanelManager()
	{
		for (auto& map : m_Panels)
			map.clear();
	}

	PanelData* PanelManager::GetPanelData(uint32_t panelID)
	{
		for (auto& panelMap : m_Panels)
		{
			if (panelMap.find(panelID) == panelMap.end())
				continue;

			return &panelMap.at(panelID);
		}

		return nullptr;
	}

	const PanelData* PanelManager::GetPanelData(uint32_t panelID) const
	{
		for (auto& panelMap : m_Panels)
		{
			if (panelMap.find(panelID) == panelMap.end())
				continue;

			return &panelMap.at(panelID);
		}

		return nullptr;
	}

	void PanelManager::RemovePanel(const char* strID)
	{
		uint32_t id = Hash::GenerateFNVHash(strID);
		for (auto& panelMap : m_Panels)
		{
			if (panelMap.find(id) == panelMap.end())
				continue;

			panelMap.erase(id);
			return;
		}

		HZ_CORE_ERROR_TAG("PanelManager", "Couldn't find panel with id '{0}'", strID);
	}

	void PanelManager::OnImGuiRender()
	{
		for (auto& panelMap : m_Panels)
		{
			for (auto& [id, panelData] : panelMap)
			{
				bool closedThisFrame = false;

				if (panelData.IsOpen)
				{
					panelData.Panel->OnImGuiRender(panelData.IsOpen);
					closedThisFrame = !panelData.IsOpen;
				}

				if (closedThisFrame)
					Serialize();
			}
		}
	}

	void PanelManager::OnEvent(Event& e)
	{
		for (auto& panelMap : m_Panels)
		{
			for (auto& [id, panelData] : panelMap)
				panelData.Panel->OnEvent(e);
		}
	}

	void PanelManager::SetSceneContext(const Ref<Scene>& context)
	{
		for (auto& panelMap : m_Panels)
		{
			for (auto& [id, panelData] : panelMap)
				panelData.Panel->SetSceneContext(context);
		}
	}

	void PanelManager::OnProjectChanged(const Ref<Project>& project)
	{
		for (auto& panelMap : m_Panels)
		{
			for (auto& [id, panelData] : panelMap)
				panelData.Panel->OnProjectChanged(project);
		}

		Deserialize();
	}

	void PanelManager::Serialize() const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Panels" << YAML::Value << YAML::BeginSeq;
		{
			for (size_t category = 0; category < m_Panels.size(); category++)
			{
				for (const auto& [panelID, panel] : m_Panels[category])
				{
					out << YAML::BeginMap;
					out << YAML::Key << "ID" << YAML::Value << panelID;
					out << YAML::Key << "Name" << YAML::Value << panel.Name;
					out << YAML::Key << "IsOpen" << YAML::Value << panel.IsOpen;
					out << YAML::EndMap;
				}
			}
		}
		out << YAML::EndSeq;

		out << YAML::EndMap;

		std::ofstream fout(FileSystem::GetPersistentStoragePath() / "EditorLayout.yaml");
		fout << out.c_str();
		fout.close();
	}

	void PanelManager::Deserialize()
	{
		std::filesystem::path layoutPath = FileSystem::GetPersistentStoragePath() / "EditorLayout.yaml";
		if (!FileSystem::Exists(layoutPath))
			return;

		std::ifstream stream(layoutPath);
		HZ_CORE_VERIFY(stream);
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["Panels"])
		{
			HZ_CONSOLE_LOG_ERROR("Failed to load EditorLayout.yaml from {} because the file appears to be corrupted!", layoutPath.parent_path().string());
			return;
		}

		for (auto panelNode : data["Panels"])
		{
			PanelData* panelData = GetPanelData(panelNode["ID"].as<uint32_t>(0));

			if (panelData == nullptr)
				continue;

			panelData->IsOpen = panelNode["IsOpen"].as<bool>(panelData->IsOpen);
		}
	}

}
