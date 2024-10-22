#include <hzpch.h>
#include "AudioCommandRegistry.h"

#include "Hazel/Core/Application.h"
#include "Hazel/Utilities/FileSystem.h"
#include "Hazel/Utilities/StringUtils.h"
#include "Hazel/Asset/AssetManager.h"

#include "yaml-cpp/yaml.h"
#include "Hazel/Utilities/SerializationMacros.h"

namespace Hazel
{
	using namespace Audio;

	std::function<void()> AudioCommandRegistry::onRegistryChange = nullptr;

	void AudioCommandRegistry::Init(WeakRef<AudioCommandRegistry>& instance)
	{
		if (!instance)
		{
			HZ_CORE_ASSERT(false, "Invalid instance of AudioCommandsRegistry.");
			return;
		}

		s_ActiveRegistry = instance;
		s_ActiveRegistry->LoadCommandRegistry(Project::GetActive()->GetAudioCommandsRegistryPath());
	}

	void AudioCommandRegistry::Shutdown()
	{
		s_ActiveRegistry = nullptr;
	}

	bool AudioCommandRegistry::AddNewCommand(ECommandType type, const char* uniqueName)
	{
		if (!s_ActiveRegistry)
		{
			HZ_CORE_ASSERT(false);
			return false;
		}

		CommandID id{ uniqueName };

		bool result = false;

		switch (type)
		{
		case ECommandType::Trigger:
			if (DoesCommandExist(type, id))	result = false;
			else
			{
				auto com = TriggerCommand();
				com.DebugName = uniqueName;
				s_ActiveRegistry->m_Triggers.emplace(id, std::move(com));
				result = true;
			}
			break;
		case ECommandType::Switch:
			if (DoesCommandExist(type, id))	result = false;
			else
			{
				auto com = SwitchCommand();
				com.DebugName = uniqueName;
				s_ActiveRegistry->m_Switches.emplace(id, std::move(com));
				result = true;
			}
			break;
		case ECommandType::State:
			if (DoesCommandExist(type, id))	result = false;
			else
			{
				auto com = StateCommand();
				com.DebugName = uniqueName;
				s_ActiveRegistry->m_States.emplace(id, std::move(com));
				result = true;
			}
			break;
		case ECommandType::Parameter:
			if (DoesCommandExist(type, id))	result = false;
			else
			{
				auto com = ParameterCommand();
				com.DebugName = uniqueName;
				s_ActiveRegistry->m_Parameters.emplace(id, std::move(com));
				result = true;
			}
			break;
		default: result = false;
		}

		if(result)
			OnRegistryChange();

		return result;
	}

	bool AudioCommandRegistry::DoesCommandExist(ECommandType type, const CommandID& commandID)
	{
		if (!s_ActiveRegistry)
		{
			HZ_CORE_ASSERT(false);
			return false;
		}

		auto isInRegistry = [](auto& registry, const CommandID& id) -> bool
		{
			if (registry.find(id) != registry.end())
			{
				return true;
			}

			return false;
		};
		switch (type)
		{
		case ECommandType::Trigger: return isInRegistry(s_ActiveRegistry->m_Triggers, commandID); break;
		case ECommandType::Switch: return isInRegistry(s_ActiveRegistry->m_Switches, commandID); break;
		case ECommandType::State: return isInRegistry(s_ActiveRegistry->m_States, commandID); break;
		case ECommandType::Parameter: return isInRegistry(s_ActiveRegistry->m_Parameters, commandID); break;
		default:
			return false;
		}
	}

	void AudioCommandRegistry::LoadCommandRegistry(const std::filesystem::path& filepath)
	{
		if (!FileSystem::Exists(filepath) || FileSystem::IsDirectory(filepath))
			return;
		
		std::ifstream stream(filepath);
		HZ_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		std::vector<YAML::Node> documents = YAML::LoadAll(strStream.str());
		YAML::Node data = documents[0];

		auto triggers	= data["Triggers"];

		/* TODO:
			auto switches		= data["Switches"];
			auto states		= data["States"];
			auto parameters	= data["Parameters"];
		*/

		if (!triggers)
		{
			HZ_CORE_ERROR("CommandRegistry appears to be corrupted!");
			return;
		}

		m_Triggers.reserve(triggers.size());

		for (auto entry : triggers)
		{
			TriggerCommand trigger;

			HZ_DESERIALIZE_PROPERTY(DebugName, trigger.DebugName, entry, std::string(""));
			
			if (trigger.DebugName.empty())
			{
				HZ_CORE_ERROR("Tried to load invalid command. Command must have a DebugName, or an ID!");
				continue;
			}
			
			auto actions = entry["Actions"];
			trigger.Actions.GetVector().reserve(actions.size());

			for (auto actionEntry : actions)
			{
				TriggerAction action;
				std::string type;
				std::string context;

				HZ_DESERIALIZE_PROPERTY(Type, type, actionEntry, std::string(""));
				HZ_DESERIALIZE_PROPERTY_ASSET(Target, action.Target, actionEntry, SoundConfig);
				HZ_DESERIALIZE_PROPERTY(Context, context, actionEntry, std::string(""));

				action.Type = Utils::AudioActionTypeFromString(type);
				action.Context = Utils::AudioActionContextFromString(context);

				trigger.Actions.PushBack(action);
			}
			m_Triggers[CommandID(trigger.DebugName.c_str())] = trigger;
		}

		OnRegistryChange();
	}

	void AudioCommandRegistry::Serialize()
	{
		if (!s_ActiveRegistry)
		{
			HZ_CORE_ASSERT(false);
			return;
		}

		s_ActiveRegistry->WriteRegistryToFile(Project::GetAudioCommandsRegistryPath());
	}

	void AudioCommandRegistry::WriteRegistryToFile(const std::filesystem::path& filepath)
	{
		if (Application::IsRuntime())
			return;

		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Triggers" << YAML::BeginSeq;

		std::map<Audio::CommandID, TriggerCommand> sortedTriggers;
		for (auto& [commandID, trigger] : m_Triggers)
			sortedTriggers[commandID] = trigger;

		for (auto& [commandID, trigger] : sortedTriggers)
		{
			out << YAML::BeginMap; // Trigger entry
			HZ_SERIALIZE_PROPERTY(DebugName, trigger.DebugName, out);

			out << YAML::Key << "Actions" << YAML::BeginSeq;
			for (auto& action : trigger.Actions.GetVector())
			{
				out << YAML::BeginMap;
				HZ_SERIALIZE_PROPERTY(Type, Utils::AudioActionTypeToString(action.Type), out);
				HZ_SERIALIZE_PROPERTY_ASSET(Target, action.Target, out);
				HZ_SERIALIZE_PROPERTY(Context, Utils::AudioActionContextToString(action.Context), out);
				out << YAML::EndMap;
			}
			out << YAML::EndSeq; // Actions
			out << YAML::EndMap; // Trigger entry
		}
		out << YAML::EndSeq;// Triggers
		out << YAML::EndMap;

		// TODO: Switches, States, Parameters

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	std::unordered_set<AssetHandle> AudioCommandRegistry::GetAllAssets()
	{
		std::unordered_set<AssetHandle> handles;

		const auto& registry = GetRegistry<Audio::TriggerCommand>();
		for (const auto& [commandID, trigger] : registry)
		{
			for (const TriggerAction& triggerAction : trigger.Actions)
			{
				if (triggerAction.Target)
					handles.insert(triggerAction.Target->Handle);
			}
		}

		return handles;
	}

	void AudioCommandRegistry::OnRegistryChange()
	{
		if (onRegistryChange)
			onRegistryChange();
	}


} // namespace Hazel
