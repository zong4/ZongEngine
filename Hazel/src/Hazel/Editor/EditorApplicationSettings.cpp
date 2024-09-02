#include "hzpch.h"
#include "EditorApplicationSettings.h"
#include "Hazel/Utilities/FileSystem.h"

#include <yaml-cpp/yaml.h>

#include <sstream>
#include <filesystem>

namespace Hazel {

	static std::filesystem::path s_EditorSettingsPath;
	
	EditorApplicationSettings& EditorApplicationSettings::Get()
	{
		static EditorApplicationSettings s_Settings;
		return s_Settings;
	}

	void EditorApplicationSettingsSerializer::Init()
	{
		s_EditorSettingsPath = std::filesystem::absolute("Config");

		if (!FileSystem::Exists(s_EditorSettingsPath))
			FileSystem::CreateDirectory(s_EditorSettingsPath);
		s_EditorSettingsPath /= "EditorApplicationSettings.yaml";

		LoadSettings();
	}

#define HZ_ENTER_GROUP(name) currentGroup = rootNode[name];
#define HZ_READ_VALUE(name, type, var, defaultValue) var = currentGroup[name].as<type>(defaultValue)

	void EditorApplicationSettingsSerializer::LoadSettings()
	{
		// Generate default settings file if one doesn't exist
		if (!FileSystem::Exists(s_EditorSettingsPath))
		{
			SaveSettings();
			return;
		}

		std::ifstream stream(s_EditorSettingsPath);
		HZ_CORE_VERIFY(stream);
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["EditorApplicationSettings"])
			return;

		YAML::Node rootNode = data["EditorApplicationSettings"];
		YAML::Node currentGroup = rootNode;

		auto& settings = EditorApplicationSettings::Get();

		HZ_ENTER_GROUP("Hazelnut");
		{
			HZ_READ_VALUE("AdvancedMode", bool, settings.AdvancedMode, false);
			HZ_READ_VALUE("HighlightUnsetMeshes", bool, settings.HighlightUnsetMeshes, true);
			HZ_READ_VALUE("TranslationSnapValue", float, settings.TranslationSnapValue, 0.5f);
			HZ_READ_VALUE("RotationSnapValue", float, settings.RotationSnapValue, 45.0f);
			HZ_READ_VALUE("ScaleSnapValue", float, settings.ScaleSnapValue, 0.5f);
		}

		HZ_ENTER_GROUP("Scripting");
		{
			HZ_READ_VALUE("ShowHiddenFields", bool, settings.ShowHiddenFields, false);
			HZ_READ_VALUE("DebuggerListenPort", int, settings.ScriptDebuggerListenPort, 2550);
		}

		HZ_ENTER_GROUP("ContentBrowser");
		{
			HZ_READ_VALUE("ShowAssetTypes", bool, settings.ContentBrowserShowAssetTypes, true);
			HZ_READ_VALUE("ThumbnailSize", int, settings.ContentBrowserThumbnailSize, 128);
		}

		stream.close();
	}

#define HZ_BEGIN_GROUP(name)\
		out << YAML::Key << name << YAML::Value << YAML::BeginMap;
#define HZ_END_GROUP() out << YAML::EndMap;

#define HZ_SERIALIZE_VALUE(name, value) out << YAML::Key << name << YAML::Value << value;

	void EditorApplicationSettingsSerializer::SaveSettings()
	{
		const auto& settings = EditorApplicationSettings::Get();

		YAML::Emitter out;
		out << YAML::BeginMap;
		HZ_BEGIN_GROUP("EditorApplicationSettings");
		{
			HZ_BEGIN_GROUP("Hazelnut");
			{
				HZ_SERIALIZE_VALUE("AdvancedMode", settings.AdvancedMode);
				HZ_SERIALIZE_VALUE("HighlightUnsetMeshes", settings.HighlightUnsetMeshes);
				HZ_SERIALIZE_VALUE("TranslationSnapValue", settings.TranslationSnapValue);
				HZ_SERIALIZE_VALUE("RotationSnapValue", settings.RotationSnapValue);
				HZ_SERIALIZE_VALUE("ScaleSnapValue", settings.ScaleSnapValue);
			}
			HZ_END_GROUP(); 
			
			HZ_BEGIN_GROUP("Scripting");
			{
				HZ_SERIALIZE_VALUE("ShowHiddenFields", settings.ShowHiddenFields);
				HZ_SERIALIZE_VALUE("DebuggerListenPort", settings.ScriptDebuggerListenPort);
			}
			HZ_END_GROUP();

			HZ_BEGIN_GROUP("ContentBrowser");
			{
				HZ_SERIALIZE_VALUE("ShowAssetTypes", settings.ContentBrowserShowAssetTypes);
				HZ_SERIALIZE_VALUE("ThumbnailSize", settings.ContentBrowserThumbnailSize);
			}
			HZ_END_GROUP();
		}
		HZ_END_GROUP();
		out << YAML::EndMap;

		std::ofstream fout(s_EditorSettingsPath);
		fout << out.c_str();
		fout.close();
	}


}
