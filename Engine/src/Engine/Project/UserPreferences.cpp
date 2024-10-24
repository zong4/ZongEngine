#include "pch.h"
#include "UserPreferences.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <sstream>

namespace Engine {

	UserPreferencesSerializer::UserPreferencesSerializer(const Ref<UserPreferences>& preferences)
		: m_Preferences(preferences)
	{
	}

	UserPreferencesSerializer::~UserPreferencesSerializer()
	{
	}

	void UserPreferencesSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "UserPrefs" << YAML::Value;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "ShowWelcomeScreen" << YAML::Value << m_Preferences->ShowWelcomeScreen;

			if (!m_Preferences->StartupProject.empty())
				out << YAML::Key << "StartupProject" << YAML::Value << m_Preferences->StartupProject;

			{
				out << YAML::Key << "RecentProjects";
				out << YAML::Value << YAML::BeginSeq;
				for (const auto&[lastOpened, projectConfig] : m_Preferences->RecentProjects)
				{
					out << YAML::BeginMap;
					out << YAML::Key << "Name" << YAML::Value << projectConfig.Name;
					out << YAML::Key << "ProjectPath" << YAML::Value << projectConfig.FilePath;
					out << YAML::Key << "LastOpened" << YAML::Value << projectConfig.LastOpened;
					out << YAML::EndMap;
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();

		m_Preferences->FilePath = filepath.string();
	}

	void UserPreferencesSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		ZONG_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["UserPrefs"])
			return;

		YAML::Node rootNode = data["UserPrefs"];
		m_Preferences->ShowWelcomeScreen = rootNode["ShowWelcomeScreen"].as<bool>();
		m_Preferences->StartupProject = rootNode["StartupProject"] ? rootNode["StartupProject"].as<std::string>() : "";

		for (auto recentProject : rootNode["RecentProjects"])
		{
			RecentProject entry;
			entry.Name = recentProject["Name"].as<std::string>();
			entry.FilePath = recentProject["ProjectPath"].as<std::string>();
			entry.LastOpened = recentProject["LastOpened"] ? recentProject["LastOpened"].as<time_t>() : time(NULL);
			m_Preferences->RecentProjects[entry.LastOpened] = entry;
		}

		stream.close();

		m_Preferences->FilePath = filepath.string();
	}

}
