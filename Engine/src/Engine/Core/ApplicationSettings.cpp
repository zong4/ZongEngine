#include "ApplicationSettings.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <iostream>

namespace Hazel {

	static void CreateDirectoriesIfNeeded(const std::filesystem::path& path)
	{
		std::filesystem::path directory = path.parent_path();
		if (!std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);
	}

	ApplicationSettings::ApplicationSettings(const std::filesystem::path& filepath)
		: m_FilePath(filepath)
	{
		Deserialize();
	}

	void ApplicationSettings::Serialize()
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Hazel Application Settings";
		out << YAML::Value;

		out << YAML::BeginMap;
		for (const auto& [key, value] : m_Settings)
			out << YAML::Key << key << YAML::Value << value;

		out << YAML::EndMap;

		out << YAML::EndSeq;

		CreateDirectoriesIfNeeded(m_FilePath);
		std::ofstream fout(m_FilePath);
		fout << out.c_str();

		fout.close();
	}

	bool ApplicationSettings::Deserialize()
	{
		std::ifstream stream(m_FilePath);
		if (!stream.good())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		auto settings = data["Hazel Application Settings"];
		if (!settings)
			return false;

		for (auto it = settings.begin(); it != settings.end(); it++)
		{
			const auto& key = it->first.as<std::string>();
			const auto& value = it->second.as<std::string>();
			m_Settings[key] = value;
		}

		stream.close();
		return true;
	}

	bool ApplicationSettings::HasKey(std::string_view key) const
	{
		return m_Settings.find(std::string(key)) != m_Settings.end();
	}

	std::string ApplicationSettings::Get(std::string_view name, const std::string& defaultValue) const
	{
		if (!HasKey(name))
			return defaultValue;

		return m_Settings.at(std::string(name));
	}

	float ApplicationSettings::GetFloat(std::string_view name, float defaultValue) const
	{
		if (!HasKey(name))
			return defaultValue;

		const std::string& string = m_Settings.at(std::string(name));
		return std::stof(string);
	}

	int ApplicationSettings::GetInt(std::string_view name, int defaultValue) const
	{
		if (!HasKey(name))
			return defaultValue;

		const std::string& string = m_Settings.at(std::string(name));
		return std::stoi(string);
	}

	void ApplicationSettings::Set(std::string_view name, std::string_view value)
	{
		m_Settings[std::string(name)] = value;
	}

	void ApplicationSettings::SetFloat(std::string_view name, float value)
	{
		m_Settings[std::string(name)] = std::to_string(value);
	}

	void ApplicationSettings::SetInt(std::string_view name, int value)
	{
		m_Settings[std::string(name)] = std::to_string(value);
	}

}
