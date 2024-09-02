#include "hzpch.h"
#include "ProjectSerializer.h"
#include "ProjectRuntimeFormat.h"

#include "Hazel/Physics/PhysicsSystem.h"
#include "Hazel/Physics/PhysicsLayer.h"
#include "Hazel/Audio/AudioEngine.h"

#include "Hazel/Utilities/YAMLSerializationHelpers.h"
#include "Hazel/Utilities/SerializationMacros.h"
#include "Hazel/Utilities/StringUtils.h"

#include "yaml-cpp/yaml.h"

#include <filesystem>

namespace Hazel {

	ProjectSerializer::ProjectSerializer(Ref<Project> project)
		: m_Project(project)
	{
	}

	void ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << m_Project->m_Config.Name;
			out << YAML::Key << "AssetDirectory" << YAML::Value << m_Project->m_Config.AssetDirectory;
			out << YAML::Key << "AssetRegistry" << YAML::Value << m_Project->m_Config.AssetRegistryPath;
			out << YAML::Key << "AudioCommandsRegistryPath" << YAML::Value << m_Project->m_Config.AudioCommandsRegistryPath;
			out << YAML::Key << "MeshPath" << YAML::Value << m_Project->m_Config.MeshPath;
			out << YAML::Key << "MeshSourcePath" << YAML::Value << m_Project->m_Config.MeshSourcePath;
			out << YAML::Key << "AnimationPath" << YAML::Value << m_Project->m_Config.AnimationPath;

			out << YAML::Key << "ScriptModulePath" << YAML::Value << m_Project->m_Config.ScriptModulePath;
			out << YAML::Key << "DefaultNamespace" << YAML::Value << m_Project->m_Config.DefaultNamespace;

			out << YAML::Key << "StartScene" << YAML::Value << m_Project->m_Config.StartScene;
			out << YAML::Key << "AutomaticallyReloadAssembly" << YAML::Value << m_Project->m_Config.AutomaticallyReloadAssembly;
			out << YAML::Key << "AutoSave" << YAML::Value << m_Project->m_Config.EnableAutoSave;
			out << YAML::Key << "AutoSaveInterval" << YAML::Value << m_Project->m_Config.AutoSaveIntervalSeconds;

			out << YAML::Key << "Audio" << YAML::Value;
			{
				out << YAML::BeginMap;
				auto userConfig = MiniAudioEngine::Get().GetUserConfiguration();
				HZ_SERIALIZE_PROPERTY(FileStreamingDurationThreshold, userConfig.FileStreamingDurationThreshold, out);
				out << YAML::EndMap;
			}

			out << YAML::Key << "Physics" << YAML::Value;
			{
				out << YAML::BeginMap;

				const auto& physicsSettings = PhysicsSystem::GetSettings();

				out << YAML::Key << "FixedTimestep" << YAML::Value << physicsSettings.FixedTimestep;
				out << YAML::Key << "Gravity" << YAML::Value << physicsSettings.Gravity;
				out << YAML::Key << "SolverPositionIterations" << YAML::Value << physicsSettings.PositionSolverIterations;
				out << YAML::Key << "SolverVelocityIterations" << YAML::Value << physicsSettings.VelocitySolverIterations;
				out << YAML::Key << "MaxBodies" << YAML::Value << physicsSettings.MaxBodies;

				out << YAML::Key << "CaptureOnPlay" << YAML::Value << physicsSettings.CaptureOnPlay;
				out << YAML::Key << "CaptureMethod" << YAML::Value << (int)physicsSettings.CaptureMethod;

				// > 1 because of the Default layer
				if (PhysicsLayerManager::GetLayerCount() > 1)
				{
					out << YAML::Key << "Layers";
					out << YAML::Value << YAML::BeginSeq;
					for (const auto& layer : PhysicsLayerManager::GetLayers())
					{
						// Never serialize the Default layer
						if (layer.LayerID == 0)
							continue;

						out << YAML::BeginMap;
						out << YAML::Key << "Name" << YAML::Value << layer.Name;
						out << YAML::Key << "CollidesWithSelf" << YAML::Value << layer.CollidesWithSelf;
						out << YAML::Key << "CollidesWith" << YAML::Value;
						out << YAML::BeginSeq;
						for (const auto& collidingLayer : PhysicsLayerManager::GetLayerCollisions(layer.LayerID))
						{
							out << YAML::BeginMap;
							out << YAML::Key << "Name" << YAML::Value << collidingLayer.Name;
							out << YAML::EndMap;
						}
						out << YAML::EndSeq;
						out << YAML::EndMap;
					}
					out << YAML::EndSeq;
				}

				out << YAML::EndMap;
			}

			out << YAML::Key << "Log" << YAML::Value;
			{
				out << YAML::BeginMap;
				auto& tags = Log::EnabledTags();
				for (auto& [name, details] : tags)
				{
					if (!name.empty()) // Don't serialize untagged log
					{
						out << YAML::Key << name << YAML::Value;
						out << YAML::BeginMap;
						{
							out << YAML::Key << "Enabled" << YAML::Value << details.Enabled;
							out << YAML::Key << "LevelFilter" << YAML::Value << Log::LevelToString(details.LevelFilter);
							out << YAML::EndMap;
						}
						
					}
				}
				out << YAML::EndMap;
			}

			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();

		m_Project->OnSerialized();
	}

	bool ProjectSerializer::SerializeRuntime(const std::filesystem::path& filepath)
	{
		ProjectInfo projectInfo;

		// Start Scene
		{
			auto& config = m_Project->m_Config;
			AssetHandle startSceneID = 0;
			if (!config.StartScene.empty())
			{
				startSceneID = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(config.StartScene);
			}

			if (startSceneID == 0)
			{
				HZ_CORE_ERROR("Error building runtime project - no start scene could be found! (StartScene: {})", config.StartScene);
				return false;
			}
			projectInfo.StartScene = startSceneID;
		}

		// Audio
		{
			auto userConfig = MiniAudioEngine::Get().GetUserConfiguration();
			projectInfo.AudioInfo.FileStreamingDurationThreshold = userConfig.FileStreamingDurationThreshold;
		}


		FileStreamWriter serializer(filepath);
		serializer.WriteRaw<ProjectInfo>(projectInfo);

		// Physics
		const auto& physicsSettings = PhysicsSystem::GetSettings();

		serializer.WriteRaw<float>(physicsSettings.FixedTimestep);
		serializer.WriteRaw<glm::vec3>(physicsSettings.Gravity);
		serializer.WriteRaw<uint32_t>(physicsSettings.PositionSolverIterations);
		serializer.WriteRaw<uint32_t>(physicsSettings.VelocitySolverIterations);
		serializer.WriteRaw<uint32_t>(physicsSettings.MaxBodies);

		uint32_t physicsLayerCount = PhysicsLayerManager::GetLayerCount();
		serializer.WriteRaw<uint32_t>(physicsLayerCount - 1); // NOTE(Yan): don't include default layer
		if (physicsLayerCount > 1)
		{
			const auto& layers = PhysicsLayerManager::GetLayers();

			// Write only names first
			for (const auto& layer : layers)
			{
				if (layer.LayerID == 0)
					continue;

				serializer.WriteString(layer.Name);
			}

			// Write remaining data
			for (const auto& layer : layers)
			{
				if (layer.LayerID == 0)
					continue;

				serializer.WriteRaw<bool>(layer.CollidesWithSelf);

				const auto& layerCollisions = PhysicsLayerManager::GetLayerCollisions(layer.LayerID);
				std::vector<std::string> collisionLayerNames(layerCollisions.size());
				for (size_t i = 0; i < layerCollisions.size(); i++)
				{
					collisionLayerNames[i] = layerCollisions[i].Name;
				}
				serializer.WriteArray(collisionLayerNames);
			}

		}

		// Logging
		// NOTE: these can be overidden in runtime with an external text file, the project file
		//       defines default levels for each tag
		{
			auto& tags = Log::EnabledTags();

			uint32_t count = 0;
			for (auto& [name, details] : tags)
			{
				if (!name.empty()) // Don't serialize untagged log
					count++;
			}
			serializer.WriteRaw<uint32_t>(count);
			for (auto& [name, details] : tags)
			{
				if (!name.empty()) // Don't serialize untagged log
				{
					serializer.WriteString(name);
					serializer.WriteRaw<bool>(details.Enabled);
					serializer.WriteRaw<uint8_t>((uint8_t)details.LevelFilter);
				}
			}
		}


		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		PhysicsLayerManager::ClearLayers();
		PhysicsLayerManager::AddLayer("Default");

		std::ifstream stream(filepath);
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Project"])
			return false;

		YAML::Node rootNode = data["Project"];
		if (!rootNode["Name"])
			return false;

		auto& config = m_Project->m_Config;
		config.Name = rootNode["Name"].as<std::string>();

		config.AssetDirectory = rootNode["AssetDirectory"].as<std::string>();
		config.AssetRegistryPath = rootNode["AssetRegistry"].as<std::string>();

		std::filesystem::path projectPath = filepath;
		config.ProjectFileName = projectPath.filename().string();
		config.ProjectDirectory = projectPath.parent_path().string();

		if (rootNode["AudioCommandsRegistryPath"])
			config.AudioCommandsRegistryPath = rootNode["AudioCommandsRegistryPath"].as<std::string>();

		config.StartScene = rootNode["StartScene"].as<std::string>("");

		config.MeshPath = rootNode["MeshPath"].as<std::string>();
		config.MeshSourcePath = rootNode["MeshSourcePath"].as<std::string>();

		config.AnimationPath = rootNode["AnimationPath"].as<std::string>("Assets/Animation");

		config.ScriptModulePath = rootNode["ScriptModulePath"].as<std::string>(config.ScriptModulePath);

		config.DefaultNamespace = rootNode["DefaultNamespace"].as<std::string>(config.Name);
		config.AutomaticallyReloadAssembly = rootNode["AutomaticallyReloadAssembly"].as<bool>(true);

		config.EnableAutoSave = rootNode["AutoSave"].as<bool>(false);
		config.AutoSaveIntervalSeconds = rootNode["AutoSaveInterval"].as<int>(300);

		// Audio
		auto audioNode = rootNode["Audio"];
		if (audioNode)
		{
			auto userConfig = MiniAudioEngine::Get().GetUserConfiguration();
			HZ_DESERIALIZE_PROPERTY(FileStreamingDurationThreshold, userConfig.FileStreamingDurationThreshold, audioNode, userConfig.FileStreamingDurationThreshold);
			MiniAudioEngine::Get().SetUserConfiguration(userConfig);
		}

		// Physics
		auto physicsNode = rootNode["Physics"];
		if (physicsNode)
		{
			auto& physicsSettings = PhysicsSystem::GetSettings();

			physicsSettings.FixedTimestep = physicsNode["FixedTimestep"].as<float>(1.0f / 60.0f);
			physicsSettings.Gravity = physicsNode["Gravity"].as<glm::vec3>(glm::vec3(0.0f, -9.81f, 0.0f));
			physicsSettings.PositionSolverIterations = physicsNode["SolverPositionIterations"].as<uint32_t>(8);
			physicsSettings.VelocitySolverIterations = physicsNode["SolverVelocityIterations"].as<uint32_t>(2);

			physicsSettings.MaxBodies = physicsNode["MaxBodies"].as<uint32_t>(1000);

			physicsSettings.CaptureOnPlay = physicsNode["CaptureOnPlay"].as<bool>(true);
			physicsSettings.CaptureMethod = (PhysicsDebugType)physicsNode["CaptureMethod"].as<int>((int)PhysicsDebugType::LiveDebug);

			auto physicsLayers = physicsNode["Layers"];

			if (!physicsLayers)
				physicsLayers = physicsNode["PhysicsLayers"]; // Temporary fix since I accidentially serialized physics layers under the wrong name until now... Will remove this in a month or so once most projects have been changed to the proper name

			if (physicsLayers)
			{
				for (auto layer : physicsLayers)
					PhysicsLayerManager::AddLayer(layer["Name"].as<std::string>(), false);

				for (auto layer : physicsLayers)
				{
					PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layer["Name"].as<std::string>());
					layerInfo.CollidesWithSelf = layer["CollidesWithSelf"].as<bool>(true);

					auto collidesWith = layer["CollidesWith"];
					if (collidesWith)
					{
						for (auto collisionLayer : collidesWith)
						{
							const auto& otherLayer = PhysicsLayerManager::GetLayer(collisionLayer["Name"].as<std::string>());
							PhysicsLayerManager::SetLayerCollision(layerInfo.LayerID, otherLayer.LayerID, true);
						}
					}
				}
			}
		}

		// Log
		auto logNode = rootNode["Log"];
		if (logNode)
		{
			auto& tags = Log::EnabledTags();
			for (auto node : logNode)
			{
				std::string name = node.first.as<std::string>();
				auto& details = tags[name];
				details.Enabled = node.second["Enabled"].as<bool>();
				details.LevelFilter = Log::LevelFromString(node.second["LevelFilter"].as<std::string>());
			}
		}

		m_Project->OnDeserialized();
		return true;
	}

	bool ProjectSerializer::DeserializeRuntime(const std::filesystem::path& filepath)
	{
		PhysicsLayerManager::ClearLayers();
		PhysicsLayerManager::AddLayer("Default");

		HZ_CORE_TRACE_TAG("Project", "Deserializing Project from {}", filepath.string());

		FileStreamReader stream(filepath);
		if (!stream.IsStreamGood())
			return false;

		ProjectInfo projectInfo;
		stream.ReadRaw<ProjectInfo>(projectInfo);
		bool validHeader = memcmp(projectInfo.Header.HEADER, "HDAT", 4) == 0;
		HZ_CORE_VERIFY(validHeader);
		if (!validHeader)
		{
			char invalidHeader[4];
			memcpy(invalidHeader, projectInfo.Header.HEADER, 4);
			HZ_CORE_ERROR_TAG("Project", "Project file has invalid header ({})", invalidHeader);
			return false;
		}

		ProjectInfo current;
		if (projectInfo.Header.Version != current.Header.Version)
		{
			HZ_CORE_ERROR_TAG("Project", "Project version {} is not compatible with current version {}", projectInfo.Header.Version, current.Header.Version);
			return false;
		}
		
		// Set project config
		auto& config = m_Project->m_Config;
		config.ProjectDirectory = filepath.parent_path().string();
		config.StartSceneHandle = projectInfo.StartScene;

		// Audio
		{
			auto userConfig = MiniAudioEngine::Get().GetUserConfiguration();
			userConfig.FileStreamingDurationThreshold = projectInfo.AudioInfo.FileStreamingDurationThreshold;
			MiniAudioEngine::Get().SetUserConfiguration(userConfig);
		}

		// Physics
		{
			auto& physicsSettings = PhysicsSystem::GetSettings();

			stream.ReadRaw<float>(physicsSettings.FixedTimestep);
			stream.ReadRaw<glm::vec3>(physicsSettings.Gravity);
			stream.ReadRaw<uint32_t>(physicsSettings.PositionSolverIterations);
			stream.ReadRaw<uint32_t>(physicsSettings.VelocitySolverIterations);
			stream.ReadRaw<uint32_t>(physicsSettings.MaxBodies);

			uint32_t physicsLayerCount = PhysicsLayerManager::GetLayerCount();
			stream.ReadRaw<uint32_t>(physicsLayerCount);
			if (physicsLayerCount > 1)
			{
				std::vector<std::string> layerNameStrings(physicsLayerCount);
				for (uint32_t i = 0; i < physicsLayerCount; i++)
				{
					stream.ReadString(layerNameStrings[i]);
					PhysicsLayerManager::AddLayer(layerNameStrings[i], false);
				}

				for (uint32_t i = 0; i < physicsLayerCount; i++)
				{
					PhysicsLayer& layerInfo = PhysicsLayerManager::GetLayer(layerNameStrings[i]);
					stream.ReadRaw<bool>(layerInfo.CollidesWithSelf);

					std::vector<std::string> collisionLayerNames;
					stream.ReadArray(collisionLayerNames);
					for (auto& collisionLayer : collisionLayerNames)
					{
						const auto& otherLayer = PhysicsLayerManager::GetLayer(collisionLayer);
						PhysicsLayerManager::SetLayerCollision(layerInfo.LayerID, otherLayer.LayerID, true);
					}

				}
			}
		}

		// Logging
		// NOTE: these can be overidden in runtime with an external text file, the project file
		//       defines default levels for each tag
		{
			auto& tags = Log::EnabledTags();

			uint32_t count;
			stream.ReadRaw<uint32_t>(count);

			for (uint32_t i = 0; i < count; i++)
			{
				std::string name;
				stream.ReadString(name);
				
				auto& details = tags[name];
				stream.ReadRaw(details.Enabled);
				stream.ReadRaw(details.LevelFilter);
			}
		}
		
		//
		// OVERRIDES
		// 
		// Hazel allows certain settings to be overridden via a YAML file called Project.yaml,
		// such as logging levels.
		const std::filesystem::path overridesFile = filepath.parent_path() / "Project.yaml";
		if (FileSystem::Exists(overridesFile))
		{
			std::ifstream stream(overridesFile);
			HZ_CORE_VERIFY(stream);
			std::stringstream strStream;
			strStream << stream.rdbuf();

			YAML::Node data = YAML::Load(strStream.str());
			if (!data["Project"])
				return false;
			
			YAML::Node rootNode = data["Project"];

			// Log
			auto logNode = rootNode["Log"];
			if (logNode)
			{
				auto& tags = Log::EnabledTags();
				for (auto node : logNode)
				{
					std::string name = node.first.as<std::string>();
					auto& details = tags[name];
					details.Enabled = node.second["Enabled"].as<bool>();
					details.LevelFilter = Log::LevelFromString(node.second["LevelFilter"].as<std::string>());
				}
			}

		}

		return true;
	}

}
