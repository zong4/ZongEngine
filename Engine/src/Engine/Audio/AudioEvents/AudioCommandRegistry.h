#pragma once

#include "CommandID.h"
#include "AudioCommands.h"

#include <unordered_set>
#include <filesystem>

namespace Engine {
	class AudioCommandRegistry : public RefCounted
	{
	public:
		template<typename Type>
		using Registry = std::unordered_map<Audio::CommandID, Type>;

		static void Init(WeakRef<AudioCommandRegistry>& instance);
		static void Shutdown();

		// This is a source of warnings under clang ("explicit specialization cannot have a storage class")
		// But I'm not touching it for now to minimize changes while porting.

		template <class Type>
		static bool AddNewCommand(const char* uniqueName) = delete;
		template<> static bool AddNewCommand<Audio::TriggerCommand>		(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Trigger, uniqueName);}
		template<> static bool AddNewCommand<Audio::SwitchCommand>		(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Switch, uniqueName);}
		template<> static bool AddNewCommand<Audio::StateCommand>		(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::State, uniqueName);}
		template<> static bool AddNewCommand<Audio::ParameterCommand>	(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Parameter, uniqueName);}

		template <class Type>
		static bool DoesCommandExist(const Audio::CommandID& commandID) = delete;
		template<> static bool DoesCommandExist<Audio::TriggerCommand>		(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Trigger, commandID); }
		template<> static bool DoesCommandExist<Audio::SwitchCommand>		(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Switch, commandID); }
		template<> static bool DoesCommandExist<Audio::StateCommand>		(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::State, commandID); }
		template<> static bool DoesCommandExist<Audio::ParameterCommand>	(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Parameter, commandID); }

		template <class Type>
		static bool RemoveCommand(const Audio::CommandID& ID) = delete;
		template<> static bool RemoveCommand<Audio::TriggerCommand>		(const Audio::CommandID& ID) { bool result = s_ActiveRegistry->m_Triggers.erase(ID);	OnRegistryChange(); return result;}
		template<> static bool RemoveCommand<Audio::SwitchCommand>		(const Audio::CommandID& ID) { bool result = s_ActiveRegistry->m_Switches.erase(ID);	OnRegistryChange(); return result;}
		template<> static bool RemoveCommand<Audio::StateCommand>		(const Audio::CommandID& ID) { bool result = s_ActiveRegistry->m_States.erase(ID);		OnRegistryChange(); return result;}
		template<> static bool RemoveCommand<Audio::ParameterCommand>	(const Audio::CommandID& ID) { bool result = s_ActiveRegistry->m_Parameters.erase(ID);	OnRegistryChange(); return result;}

		template <class Type>
		static Type& GetCommand(const Audio::CommandID& ID) = delete;
		template<> static Audio::TriggerCommand&	GetCommand<Audio::TriggerCommand>		(const Audio::CommandID& ID) { return s_ActiveRegistry->m_Triggers.at(ID);}
		template<> static Audio::SwitchCommand&		GetCommand<Audio::SwitchCommand>		(const Audio::CommandID& ID) { return s_ActiveRegistry->m_Switches.at(ID);}
		template<> static Audio::StateCommand&		GetCommand<Audio::StateCommand>			(const Audio::CommandID& ID) { return s_ActiveRegistry->m_States.at(ID);}
		template<> static Audio::ParameterCommand&	GetCommand<Audio::ParameterCommand>		(const Audio::CommandID& ID) { return s_ActiveRegistry->m_Parameters.at(ID);}

		template <class Type>
		static const Registry<Type>& GetRegistry() {}
		template<> static const Registry<Audio::TriggerCommand>&	GetRegistry<Audio::TriggerCommand>()	{ return s_ActiveRegistry->m_Triggers; }
		template<> static const Registry<Audio::SwitchCommand>&		GetRegistry<Audio::SwitchCommand>()		{ return s_ActiveRegistry->m_Switches; }
		template<> static const Registry<Audio::StateCommand>&		GetRegistry<Audio::StateCommand>()		{ return s_ActiveRegistry->m_States; }
		template<> static const Registry<Audio::ParameterCommand>&	GetRegistry<Audio::ParameterCommand>()	{ return s_ActiveRegistry->m_Parameters; }

		static void Serialize();
		static WeakRef<AudioCommandRegistry>& GetActive() { return s_ActiveRegistry; }
		void WriteRegistryToFile(const std::filesystem::path& filepath);

		static std::unordered_set<AssetHandle> GetAllAssets();
	private:
		static bool DoesCommandExist(Audio::ECommandType type, const Audio::CommandID& commandID);
		static bool AddNewCommand(Audio::ECommandType type, const char* uniqueName);
		static void OnRegistryChange();

		friend class Project;
		friend class Ref<AudioCommandRegistry>;
		AudioCommandRegistry() = default;
		void LoadCommandRegistry(const std::filesystem::path& filepath);

	public:
		static std::function<void()> onRegistryChange;

	private:
		Registry<Audio::TriggerCommand> m_Triggers;
		Registry<Audio::SwitchCommand> m_Switches;
		Registry<Audio::StateCommand> m_States;
		Registry<Audio::ParameterCommand> m_Parameters;

		inline static WeakRef<AudioCommandRegistry> s_ActiveRegistry;
	};

} // namespace Engine

