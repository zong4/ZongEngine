#pragma once

#include "CommandID.h"
#include "AudioCommands.h"

#include <unordered_set>
#include <filesystem>

namespace Hazel {
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
		template<class Type>
		requires(std::is_same_v<Type, Audio::TriggerCommand>)
		static bool AddNewCommand(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Trigger, uniqueName);}
		template<class Type>
		requires(std::is_same_v<Type, Audio::SwitchCommand>)
		static bool AddNewCommand(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Switch, uniqueName);}
		template<class Type>
		requires(std::is_same_v<Type, Audio::StateCommand>)
		static bool AddNewCommand(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::State, uniqueName);}
		template<class Type>
		requires(std::is_same_v<Type, Audio::ParameterCommand>)
		static bool AddNewCommand(const char* uniqueName) { return AddNewCommand(Audio::ECommandType::Parameter, uniqueName);}

		template <class Type>
		static bool DoesCommandExist(const Audio::CommandID& commandID) = delete;
		template<class Type>
		requires(std::is_same_v<Type, Audio::TriggerCommand>)
		static bool DoesCommandExist(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Trigger, commandID); }
		template<class Type>
		requires(std::is_same_v<Type, Audio::SwitchCommand>)
		static bool DoesCommandExist(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Switch, commandID); }
		template<class Type>
		requires(std::is_same_v<Type, Audio::StateCommand>)
		static bool DoesCommandExist(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::State, commandID); }
		template<class Type>
		requires(std::is_same_v<Type, Audio::ParameterCommand>)
		static bool DoesCommandExist(const Audio::CommandID& commandID) { return DoesCommandExist(Audio::ECommandType::Parameter, commandID); }

		template <class Type>
		static bool RemoveCommand(const Audio::CommandID& ID) = delete;
		template<class Type>
		requires(std::is_same_v<Type, Audio::TriggerCommand>)
		static bool RemoveCommand(const Audio::CommandID& ID) { bool result = s_ActiveRegistry->m_Triggers.erase(ID);	OnRegistryChange(); return result;}
		template<class Type>
		requires(std::is_same_v<Type, Audio::SwitchCommand>)
		static bool RemoveCommand(const Audio::CommandID& ID) { bool result = s_ActiveRegistry->m_Switches.erase(ID);	OnRegistryChange(); return result;}
		template<class Type>
		requires(std::is_same_v<Type, Audio::StateCommand>)
		static bool RemoveCommand(const Audio::CommandID& ID) { bool result = s_ActiveRegistry->m_States.erase(ID);		OnRegistryChange(); return result;}
		template<class Type>
		requires(std::is_same_v<Type, Audio::ParameterCommand>)
		static bool RemoveCommand(const Audio::CommandID& ID) { bool result = s_ActiveRegistry->m_Parameters.erase(ID);	OnRegistryChange(); return result;}

		template <class Type>
		static Type& GetCommand(const Audio::CommandID& ID) = delete;
		template<class Type>
		requires(std::is_same_v<Type, Audio::TriggerCommand>)
		static Audio::TriggerCommand&   GetCommand(const Audio::CommandID& ID) { return s_ActiveRegistry->m_Triggers.at(ID);}
		template<class Type>
		requires(std::is_same_v<Type, Audio::SwitchCommand>)
		static Audio::SwitchCommand&    GetCommand(const Audio::CommandID& ID) { return s_ActiveRegistry->m_Switches.at(ID);}
		template<class Type>
		requires(std::is_same_v<Type, Audio::StateCommand>)
		static Audio::StateCommand&     GetCommand(const Audio::CommandID& ID) { return s_ActiveRegistry->m_States.at(ID);}
		template<class Type>
		requires(std::is_same_v<Type, Audio::ParameterCommand>)
		static Audio::ParameterCommand&	GetCommand(const Audio::CommandID& ID) { return s_ActiveRegistry->m_Parameters.at(ID);}

		template <class Type>
		static const Registry<Type>& GetRegistry() {}
		template<class Type>
		requires(std::is_same_v<Type, Audio::TriggerCommand>)
		static const Registry<Audio::TriggerCommand>&	GetRegistry() { return s_ActiveRegistry->m_Triggers; }
		template<class Type>
		requires(std::is_same_v<Type, Audio::SwitchCommand>)
		static const Registry<Audio::SwitchCommand>&	GetRegistry() { return s_ActiveRegistry->m_Switches; }
		template<class Type>
		requires(std::is_same_v<Type, Audio::StateCommand>)
		static const Registry<Audio::StateCommand>&		GetRegistry() { return s_ActiveRegistry->m_States; }
		template<class Type>
		requires(std::is_same_v<Type, Audio::ParameterCommand>)
		static const Registry<Audio::ParameterCommand>&	GetRegistry() { return s_ActiveRegistry->m_Parameters; }

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

} // namespace Hazel

