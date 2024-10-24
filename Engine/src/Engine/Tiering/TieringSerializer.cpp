#include "TieringSerializer.h"

#include "yaml-cpp/yaml.h"

#include <fstream>

namespace Engine {

	static void CreateDirectoriesIfNeeded(const std::filesystem::path& path)
	{
		std::filesystem::path directory = path.parent_path();
		if (!directory.empty() && !std::filesystem::exists(directory))
			std::filesystem::create_directories(directory);
	}

	void TieringSerializer::Serialize(const Tiering::TieringSettings& tieringSettings, const std::filesystem::path& filepath)
	{
		using namespace Tiering::Renderer;

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "TieringSettings";
		out << YAML::Value;

		out << YAML::BeginMap;
		out << YAML::Key << "RendererScale" << YAML::Value << tieringSettings.RendererTS.RendererScale;
		out << YAML::Key << "Windowed" << YAML::Value << tieringSettings.RendererTS.Windowed;
		out << YAML::Key << "VSync" << YAML::Value << tieringSettings.RendererTS.VSync;
		out << YAML::Key << "ShadowQuality" << YAML::Value << Utils::ShadowQualitySettingToString(tieringSettings.RendererTS.ShadowQuality);
		out << YAML::Key << "ShadowResolution" << YAML::Value << Utils::ShadowResolutionSettingToString(tieringSettings.RendererTS.ShadowResolution);
		out << YAML::Key << "AmbientOcclusionQuality" << YAML::Value << Utils::AmbientOcclusionQualitySettingToString(tieringSettings.RendererTS.AOQuality);
		out << YAML::Key << "SSRQuality" << YAML::Value << Utils::SSRQualitySettingToString(tieringSettings.RendererTS.SSRQuality);
		out << YAML::EndMap;

		out << YAML::EndSeq;

		CreateDirectoriesIfNeeded(filepath);
		std::ofstream fout(filepath);
		fout << out.c_str();

		fout.close();
	}

	bool TieringSerializer::Deserialize(Tiering::TieringSettings& outTieringSettings, const std::filesystem::path& filepath)
	{
		using namespace Tiering::Renderer;

		std::ifstream stream(filepath);
		if (!stream.good())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());

		auto tieringSettings = data["TieringSettings"];
		if (!tieringSettings)
			return false;

		if (tieringSettings["RendererScale"])
			outTieringSettings.RendererTS.RendererScale = tieringSettings["RendererScale"].as<float>();
		if (tieringSettings["Windowed"])
			outTieringSettings.RendererTS.Windowed = tieringSettings["Windowed"].as<bool>();
		if (tieringSettings["VSync"])
			outTieringSettings.RendererTS.VSync = tieringSettings["VSync"].as<bool>();
		if (tieringSettings["ShadowQuality"])
			outTieringSettings.RendererTS.ShadowQuality = Utils::ShadowQualitySettingFromString(tieringSettings["ShadowQuality"].as<std::string>());
		if (tieringSettings["ShadowResolution"])
			outTieringSettings.RendererTS.ShadowResolution = Utils::ShadowResolutionSettingFromString(tieringSettings["ShadowResolution"].as<std::string>());

		if (tieringSettings["AmbientOcclusionQuality"])
		{
			outTieringSettings.RendererTS.AOQuality = Utils::AmbientOcclusionQualitySettingFromString(tieringSettings["AmbientOcclusionQuality"].as<std::string>());
		}
		else if (tieringSettings["AmbientOcclusion"]) // Older
		{
			bool enableAO = tieringSettings["AmbientOcclusion"].as<bool>();
			outTieringSettings.RendererTS.AOQuality = enableAO ? Tiering::Renderer::AmbientOcclusionQualitySetting::High : Tiering::Renderer::AmbientOcclusionQualitySetting::None;
		}

		if (tieringSettings["SSRQuality"])
			outTieringSettings.RendererTS.SSRQuality = Utils::SSRQualitySettingFromString(tieringSettings["SSRQuality"].as<std::string>());

		stream.close();
		return true;
	}

}
