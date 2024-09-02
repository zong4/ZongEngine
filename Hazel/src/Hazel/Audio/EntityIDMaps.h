#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/Scene/Scene.h"
#include "AudioEvents/AudioCommands.h"
#include "AudioEvents/CommandID.h"
#include "AudioComponent.h"

#include <optional>
#include <shared_mutex>
#include <numeric>
#include <variant>

namespace Hazel {
	/* ---------------------------------------------------------------
		Structure that provides thread safe access to map of ScenesIDs,
		their EntityIDs and associated to the Entities objects.
		--------------------------------------------------------------
	*/
	template<typename T>
	struct EntityIDMap
	{
		void Add(UUID sceneID, UUID entityID, T object)
		{
			std::scoped_lock lock{ m_Mutex };
			m_EntityIDMap[sceneID][entityID] = object;
		}

		void Remove(UUID sceneID, UUID entityID)
		{
			std::scoped_lock lock{ m_Mutex };
			HZ_CORE_ASSERT(m_EntityIDMap.at(sceneID).find(entityID) != m_EntityIDMap.at(sceneID).end(),
				"Could not find entityID in the registry to remove.");

			m_EntityIDMap.at(sceneID).erase(entityID);
			if (m_EntityIDMap.at(sceneID).empty())
				m_EntityIDMap.erase(sceneID);
		}

		std::optional<T> Get(UUID sceneID, UUID entityID) const
		{
			std::shared_lock lock{ m_Mutex };
			if (m_EntityIDMap.find(sceneID) != m_EntityIDMap.end())
			{
				if (m_EntityIDMap.at(sceneID).find(entityID) != m_EntityIDMap.at(sceneID).end())
				{
					return std::optional<T>(m_EntityIDMap.at(sceneID).at(entityID));
				}
			}
			return std::optional<T>();
		}

		std::optional<std::unordered_map<UUID, T>> Get(UUID sceneID) const
		{
			std::shared_lock lock{ m_Mutex };
			if (m_EntityIDMap.find(sceneID) != m_EntityIDMap.end())
			{
				return std::optional<std::unordered_map<UUID, T>>(m_EntityIDMap.at(sceneID));
			}
			return std::optional<std::unordered_map<UUID, T>>();
		}

		void Clear(UUID sceneID)
		{
			std::scoped_lock lock{ m_Mutex };
			if (sceneID == 0)
			{
				m_EntityIDMap.clear();
			}
			else if (m_EntityIDMap.find(sceneID) != m_EntityIDMap.end())
			{
				m_EntityIDMap.at(sceneID).clear();
				m_EntityIDMap.erase(sceneID);
			}
		}

		uint64_t Count(UUID sceneID) const
		{
			std::shared_lock lock{ m_Mutex };
			if (m_EntityIDMap.find(sceneID) != m_EntityIDMap.end())
				return m_EntityIDMap.at(sceneID).size();
			else
				return 0;
		}
	protected:
		mutable std::shared_mutex m_Mutex;
	private:
		std::unordered_map<UUID, std::unordered_map<UUID, T>> m_EntityIDMap;
	};


	/*  =================================================================
		Registry to keep track of all Audio Components in the game engine

		Provides easy access to Audio Components from AudioEngine
		-----------------------------------------------------------------
	*/
	struct AudioComponentRegistry : public EntityIDMap<Entity>
	{
		AudioComponent* GetAudioComponent(UUID sceneID, uint64_t audioComponentID) const;
		Entity GetEntity(UUID sceneID, uint64_t audioComponentID) const;
	};

	/*
	template<class T>
	struct ActiveCommandsMap
	{
	public:
		ActiveCommandsMap() {};
		ActiveCommandsMap(const ActiveCommandsMap&) = delete;

		using CommandScoped = Scope<T>;

		//===============================================================================================

		uint64_t GetCommandCount(UUID sceneID) const
		{
			return m_ActiveCommands.Count(sceneID);
		}

		std::optional<std::vector<CommandScoped>*> GetCommands(UUID sceneID, UUID objectID) const
		{
			return m_ActiveCommands.Get(sceneID, objectID);
		}

		virtual void AddCommand(UUID sceneID, UUID objectID, T* command)
		{
			if (auto* commands = m_ActiveCommands.Get(sceneID, objectID).value_or(nullptr))
			{
				commands->push_back(CommandScoped(command));
			}
			else
			{
				auto newCommandsList = new std::vector<CommandScoped>();
				newCommandsList->push_back(CommandScoped(command));
				m_ActiveCommands.Add(sceneID, objectID, newCommandsList);
			}
		}

		// @returns true - if `command` was found and removed from the registry.
		virtual bool RemoveCommand(UUID sceneID, UUID objectID, T* command)
		{
			if (auto* commands = m_ActiveCommands.Get(sceneID, objectID).value_or(nullptr))
			{
				commands->erase(std::find_if(commands->begin(), commands->end(), [command](CommandScoped& itCommand) {return itCommand.get() == command; }));

				if (commands->empty())
				{
					delete* m_ActiveCommands.Get(sceneID, objectID);
					m_ActiveCommands.Remove(sceneID, objectID);
				}

				return true;
			}

			return false;
		}

	protected:
		EntityIDMap<std::vector<CommandScoped>*> m_ActiveCommands;
	};


	struct ActiveTriggersRegistry : private ActiveCommandsMap<TriggerCommand>
	{
	public:
		ActiveTriggersRegistry();
		ActiveTriggersRegistry(const ActiveTriggersRegistry&) = delete;

		using TriggerScoped = Scope<TriggerCommand>;

		//===============================================================================================

		uint64_t Count(UUID sceneID);

		std::optional<std::vector<TriggerScoped>*> GetCommands(UUID sceneID, UUID objectID) const;
		void AddTrigger(UUID sceneID, UUID objectID, TriggerCommand* trigger);

		// @returns true - if `trigger` was found and removed from the registry./
		bool RemoveTrigger(UUID sceneID, UUID objectID, TriggerCommand* trigger);

		// @returns true - if Trigger was fully handled.
		bool OnTriggerActionHandled(UUID sceneID, UUID objectID, TriggerCommand* trigger);

		// @returns true - if Trigger owning Play action of the `soundSource` was fully handled.
		bool OnPlaybackActionFinished(UUID sceneID, UUID objectID, Ref<SoundConfig> soundSource);

	private:
		bool RefreshActions(TriggerCommand* trigger);
		bool RefreshTriggers(UUID sceneID, UUID objectID);

	private:
		mutable std::shared_mutex m_Mutex;
	};
	*/

	namespace Audio {
		// Event is an active execution instance of a Command
		struct EventID
		{
			EventID() : m_ID(++s_ID) {}
			EventID(uint32_t ID) : m_ID(ID) {}

			constexpr bool operator==(const EventID& other) const noexcept { return m_ID == other.m_ID; }
			constexpr operator bool() const { return (bool)m_ID; }
			constexpr operator uint32_t() const { return m_ID; }

			static constexpr inline uint32_t INVALID = 0;
			constexpr bool IsValid() const { return m_ID != INVALID; }

		private:
			uint32_t m_ID;
			static inline std::atomic<uint32_t> s_ID{ 0 };
		};
	} // namespace Audio
} // namespace Hazel;

namespace std {
	template <>
	struct hash<Hazel::Audio::EventID>
	{
		std::size_t operator()(const Hazel::Audio::EventID& id) const noexcept
		{
			return hash<uint32_t>()((uint32_t)id);
		}
	};
}

namespace Hazel {
	namespace Audio {

        using SourceID = int;         // ID of the sound source
		static constexpr SourceID INVALID_SOURCE_ID = -1;

        struct EventInfo
        {
            using StateObject = std::variant<std::monostate,
                TriggerCommand,
                SwitchCommand,
                StateCommand,
                ParameterCommand>;

            EventInfo(const CommandID& commandID, UUID objectID);
            EventInfo()
                : commandID("")
            {};

            EventID EventID;										// ID of the Event instance
            CommandID commandID;									// calling Command, used to retrieve instructions and list of Actions to perform
            UUID ObjectID;											// context Object of the Event
            std::vector<SourceID> ActiveSources;					// Active Sources associated with the Playback Instance

            std::shared_ptr<StateObject> CommandState{ nullptr };	// Object to track execution of the Actions
        };

        // Events and associated info
        struct EventRegistry
        {
        public:
            uint32_t Count() const;
            // @returns number of Sources associated to an Event
            uint32_t GetNumberOfSources(EventID eventID) const;

            EventID Add(EventInfo& eventInfo);
            bool Remove(EventID eventID);

            // Associate Sound Source with the Event
            bool AddSource(EventID eventID, SourceID sourceID);
            // @returns true - if eventID doesn't have any sourceIDs left and was removed from the registry
            bool RemoveSource(EventID eventID, SourceID sourceID);

            EventInfo Get(EventID eventID) const;

        private:
            mutable std::shared_mutex m_Mutex;
            std::unordered_map<EventID, EventInfo> m_PlaybackInstances;
        };


        // Objects and associated Events
        struct ObjectEventRegistry //? not used
        {
        public:
            uint32_t Count() const;
            uint32_t GetTotalPlaybackCount() const;

            bool Add(UUID objectID, EventID eventID);
            // @returns true - if objectID doesn't have any eventIDs left and was removed from the registry
            bool Remove(UUID objectID, EventID eventID);
            bool RemoveObject(UUID objectID);

            uint32_t GetNumberOfEvents(UUID objectID) const;
            std::vector<EventID> GetEvents(UUID objectID)  const;

        private:
            mutable std::shared_mutex m_Mutex;
            std::unordered_map<UUID, std::vector<EventID>> m_Objects;
        };


        // Objects and associated active Sounds
        struct ObjectSourceRegistry	//? not used
        {
        public:
            uint32_t Count() const;
            uint32_t GetTotalPlaybackCount() const;

            bool Add(UUID objectID, SourceID sourceID);
            // @returns true - if objectID doesn't have any sourceIDs left and was removed from the registry
            bool Remove(UUID objectID, SourceID sourceID);
            bool RemoveObject(UUID objectID);

            uint32_t GetNumberOfActiveSounds(UUID objectID) const;
            std::optional<std::vector<SourceID>> GetActiveSounds(UUID objectID) const;

        private:
            mutable std::shared_mutex m_Mutex;
            std::unordered_map<UUID, std::vector<SourceID>> m_Objects;
        };

    } // namespace Audio

} // namespace Hazel

//namespace std {
//	// Implementation
//	 std::size_t hash<Hazel::Audio::EventID>::operator()(const Hazel::Audio::EventID& id) const noexcept { return hash<uint32_t>()(id.m_ID); }
//}
