#include "pch.h"
#include "EditorApplicationSettings.h"
#include "Engine/Utilities/FileSystem.h"

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

#define ZONG_ENTER_GROUP(name) currentGroup = rootNode[name];
#define ZONG_READ_VALUE(name, type, var, defaultValue) var = currentGroup[name].as<type>(defaultValue)

	void EditorApplicationSettingsSerializer::LoadSettings()
	{
		// Generate default settings file if one doesn't exist
		if (!FileSystem::Exists(s_EditorSettingsPath))
		{
			SaveSettings();
			return;
		}

		std::ifstream stream(s_EditorSettingsPath);
		ZONG_CORE_VERIFY(stream);
		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node data = YAML::Load(ss.str());
		if (!data["EditorApplicationSettings"])
			return;

		YAML::Node rootNode = data["EditorApplicationSettings"];
		YAML::Node currentGroup = rootNode;

		auto& settings = EditorApplicationSettings::Get();

		ZONG_ENTER_GROUP("Editor");
		{
			ZONG_READ_VALUE("AdvancedMode", bool, settings.AdvancedMode, false);
			ZONG_READ_VALUE("HighlightUnsetMeshes", bool, settings.HighlightUnsetMeshes, true);
			ZONG_READ_VALUE("TranslationSnapValue", float, settings.TranslationSnapValue, 0.5f);
			ZONG_READ_VALUE("RotationSnapValue", float, settings.RotationSnapValue, 45.0f);
			ZONG_READ_VALUE("ScaleSnapValue", float, settings.ScaleSnapValue, 0.5f);
		}

		ZONG_ENTER_GROUP("Scripting");
		{
			ZONG_READ_VALUE("ShowHiddenFields", bool, settings.ShowHiddenFields, false);
			ZONG_READ_VALUE("DebuggerListenPort", int, settings.ScriptDebuggerListenPort, 2550);
		}

		ZONG_ENTER_GROUP("ContentBrowser");
		{
			ZONG_READ_VALUE("ShowAssetTypes", bool, settings.ContentBrowserShowAssetTypes, true);
			ZONG_READ_VALUE("ThumbnailSize", int, settings.ContentBrowserThumbnailSize, 128);
		}

		stream.close();
	}

#define ZONG_BEGIN_GROUP(name)\
		out << YAML::Key << name << YAML::Value << YAML::BeginMap;
#define ZONG_END_GROUP() out << YAML::EndMap;

#define ZONG_SERIALIZE_VALUE(name, value) out << YAML::Key << name << YAML::Value << value;

	void EditorApplicationSettingsSerializer::SaveSettings()
	{
		const auto& settings = EditorApplicationSettings::Get();

		YAML::Emitter out;
		out << YAML::BeginMap;
		ZONG_BEGIN_GROUP("EditorApplicationSettings");
		{
			ZONG_BEGIN_GROUP("Editor");
			{
				ZONG_SERIALIZE_VALUE("AdvancedMode", settings.AdvancedMode);
				ZONG_SERIALIZE_VALUE("HighlightUnsetMeshes", settings.HighlightUnsetMeshes);
				ZONG_SERIALIZE_VALUE("TranslationSnapValue", settings.TranslationSnapValue);
				ZONG_SERIALIZE_VALUE("RotationSnapValue", settings.RotationSnapValue);
				ZONG_SERIALIZE_VALUE("ScaleSnapValue", settings.ScaleSnapValue);
			}
			ZONG_END_GROUP(); 
			
			ZONG_BEGIN_GROUP("Scripting");
			{
				ZONG_SERIALIZE_VALUE("ShowHiddenFields", settings.ShowHiddenFields);
				ZONG_SERIALIZE_VALUE("DebuggerListenPort", settings.ScriptDebuggerListenPort);
			}
			ZONG_END_GROUP();

			ZONG_BEGIN_GROUP("ContentBrowser");
			{
				ZONG_SERIALIZE_VALUE("ShowAssetTypes", settings.ContentBrowserShowAssetTypes);
				ZONG_SERIALIZE_VALUE("ThumbnailSize", settings.ContentBrowserThumbnailSize);
			}
			ZONG_END_GROUP();
		}
		ZONG_END_GROUP();
		out << YAML::EndMap;

		std::ofstream fout(s_EditorSettingsPath);
		fout << out.c_str();
		fout.close();
	}


}
