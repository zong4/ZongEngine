#include <pch.h>
#include "EntityIDMaps.h"
#include "AudioEvents/CommandID.h"

namespace Hazel
{
	AudioComponent* AudioComponentRegistry::GetAudioComponent(Hazel::UUID sceneID, uint64_t audioComponentID) const
	{
		std::optional<Hazel::Entity> o = Get(sceneID, audioComponentID);
		if (o.has_value())
			return &o->GetComponent<AudioComponent>();
		else
		{
			ZONG_CORE_ASSERT("Component was not found in registry");
			return nullptr;
		}
	}

	Hazel::Entity AudioComponentRegistry::GetEntity(Hazel::UUID sceneID, uint64_t audioComponentID) const
	{
		return Get(sceneID, Hazel::UUID(audioComponentID)).value_or(Hazel::Entity());
	}

	//===============================================================================================

	/*
	Audio::ActiveTriggersRegistry::ActiveTriggersRegistry() {}

	using TriggerScoped = Hazel::Scope<TriggerCommand>;

	uint64_t ActiveTriggersRegistry::Count(Hazel::UUID sceneID)
	{
		return GetCommandCount(sceneID);
	}

	std::optional<std::vector<TriggerScoped>*> ActiveTriggersRegistry::GetCommands(Hazel::UUID sceneID, Hazel::UUID objectID) const
	{
		return m_ActiveCommands.Get(sceneID, objectID);
	}

	void ActiveTriggersRegistry::AddTrigger(Hazel::UUID sceneID, Hazel::UUID objectID, TriggerCommand* trigger)
	{
		if (auto* triggers = m_ActiveCommands.Get(sceneID, objectID).value_or(nullptr))
		{
			triggers->push_back(Hazel::Scope<TriggerCommand>(trigger));
		}
		else
		{
			auto newTriggerList = new std::vector<std::unique_ptr<TriggerCommand>>();
			newTriggerList->push_back(Hazel::Scope<TriggerCommand>(trigger));
			m_ActiveCommands.Add(sceneID, objectID, newTriggerList);
		}
	}

	bool ActiveTriggersRegistry::RemoveTrigger(Hazel::UUID sceneID, Hazel::UUID objectID, TriggerCommand* trigger)
	{
		if (auto* triggers = m_ActiveCommands.Get(sceneID, objectID).value_or(nullptr))
		{
			triggers->erase(std::find_if(triggers->begin(), triggers->end(), [trigger](TriggerScoped& itTrigger) {return itTrigger.get() == trigger; }));
			if (triggers->empty())
			{
				delete triggers;
				m_ActiveCommands.Remove(sceneID, objectID);
			}

			return true;
		}

		return false;
	}

	bool ActiveTriggersRegistry::OnTriggerActionHandled(Hazel::UUID sceneID, Hazel::UUID objectID, TriggerCommand* trigger)
	{
		if (RefreshActions(trigger))
			return RefreshTriggers(sceneID, objectID);
		else
			return false;
	}

	bool ActiveTriggersRegistry::OnPlaybackActionFinished(Hazel::UUID sceneID, Hazel::UUID objectID, Hazel::Ref<SoundConfig> soundSource)
	{
		if (auto* triggers = m_ActiveCommands.Get(sceneID, objectID).value_or(nullptr))
		{
			bool commandFullyHandled = false;

			auto it = std::find_if(triggers->begin(), triggers->end(), [this, &commandFullyHandled, sceneID, objectID, soundSource](TriggerScoped& trigger)
				{
					const auto it = std::find_if(trigger->Actions.begin(),
												trigger->Actions.end(), [soundSource](TriggerAction& action) {
																			if (action.Target == soundSource) 
																			{
																				action.Handled = true;
																				return true;
																			}
																			return false; });

					if(it != trigger->Actions.end())
					{
						ZONG_CORE_INFO("removed play trigger");
						if (RefreshActions(trigger.get()))
							commandFullyHandled = true;

						// Need to return from the 'find', otherwise we're going to mark `handled`
						// more playback instances with the same sound source.
						return true;
					}

					return false;
				});

			if (commandFullyHandled)
				return RefreshTriggers(sceneID, objectID);
		}
		return false;
	}

	//===============================================================================================
	bool ActiveTriggersRegistry::RefreshActions(TriggerCommand* trigger)
	{
		// Remove handled events from ActiveEvents
		const auto it = std::remove_if(trigger->Actions.begin(), trigger->Actions.end(), [](TriggerAction& command) { return command.Handled; });
		if (it != trigger->Actions.end())
			trigger->Actions.erase(it, trigger->Actions.end());

		if (trigger->Actions.empty())
		{
			trigger->Handled = true;
			return true;
		}
		return false;
	}

	bool ActiveTriggersRegistry::RefreshTriggers(Hazel::UUID sceneID, Hazel::UUID objectID)
	{
		bool commandFullyHandled = false;

		// If all of the Actions have been handled, remove Trigger from the object local Commands list.

		if (auto* objectTriggers = m_ActiveCommands.Get(sceneID, objectID).value_or(nullptr))
		{
			std::scoped_lock lock{ m_Mutex };

			const auto itr = std::remove_if(objectTriggers->begin(), objectTriggers->end(), [](TriggerScoped& trigger) { return trigger->Handled; });
			
			if (itr != objectTriggers->end())
			{
				const int num = objectTriggers->end() - itr;
				ZONG_CORE_INFO("Deleting finished commands, removed: {0}", num); //? for soem reasong this throws: itr->get()->DebugName
				objectTriggers->erase(itr, objectTriggers->end());

				commandFullyHandled = true;
			}

			// If all of the Commands on the object have been handled, remove object entry from the ActiveEvents list
			if (objectTriggers->empty())
			{
				ZONG_CORE_INFO("All Commands on object {0} handled.", objectID);
				delete objectTriggers;
				m_ActiveCommands.Remove(sceneID, objectID);
			}
		}

		return commandFullyHandled;
	}
	*/

	//===============================================================================================
	namespace Audio
	{

		EventInfo::EventInfo(const CommandID& comdID, Hazel::UUID objectID)
			: commandID(comdID), ObjectID(objectID)
		{
		}

		//===============================================================================================

		EventID EventRegistry::Add(EventInfo& eventInfo)
		{
			std::scoped_lock lock{ m_Mutex };

			EventID eventID;
			eventInfo.EventID = eventID;
			m_PlaybackInstances[eventID] = eventInfo;
			return eventID;
		}

		bool EventRegistry::Remove(EventID eventID)
		{
			std::scoped_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_PlaybackInstances.find(eventID) != m_PlaybackInstances.end());

			return m_PlaybackInstances.erase(eventID);
		}

		bool EventRegistry::AddSource(EventID eventID, SourceID sourceID)
		{
			std::scoped_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_PlaybackInstances.find(eventID) != m_PlaybackInstances.end());

			auto& sources = m_PlaybackInstances.at(eventID).ActiveSources;
			sources.push_back(sourceID);
			return true;
		}

		uint32_t EventRegistry::Count() const
		{
			std::shared_lock lock{ m_Mutex };

			return (uint32_t)m_PlaybackInstances.size();
		}

		uint32_t EventRegistry::GetNumberOfSources(EventID eventID) const
		{
			std::shared_lock lock{ m_Mutex };

			//ZONG_CORE_ASSERT(m_PlaybackInstances.find(eventID) != m_PlaybackInstances.end());

			if (m_PlaybackInstances.find(eventID) != m_PlaybackInstances.end())
				return (uint32_t)m_PlaybackInstances.at(eventID).ActiveSources.size();
			else
				return 0;
		}

		bool EventRegistry::RemoveSource(EventID eventID, SourceID sourceID)
		{
			std::scoped_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_PlaybackInstances.find(eventID) != m_PlaybackInstances.end());

			auto& sources = m_PlaybackInstances.at(eventID).ActiveSources;
			auto it = std::find(sources.begin(), sources.end(), sourceID);

			if (it == sources.end())
				ZONG_CORE_ERROR("Audio. EventRegistry. Attempted to remove source that's not associated to any Event in registry.");
			else
				sources.erase(it);

			return sources.empty();
			//? Should not remove EventInfo when last SourceID removed,
			//? there might still be active Events that are not referencing any Sound Source
			//m_PlaybackInstances.erase(eventID);
		}

		EventInfo EventRegistry::Get(EventID eventID) const
		{
			std::shared_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_PlaybackInstances.find(eventID) != m_PlaybackInstances.end());

			return m_PlaybackInstances.at(eventID);
		}


		//===============================================================================================

		uint32_t ObjectEventRegistry::Count() const
		{
			std::shared_lock lock{ m_Mutex };

			return (uint32_t)m_Objects.size();
		}

		uint32_t ObjectEventRegistry::GetTotalPlaybackCount() const
		{
			std::shared_lock lock{ m_Mutex };

			return (uint32_t)std::accumulate(m_Objects.begin(), m_Objects.end(), 0,
				[](uint32_t prior, const std::pair<Hazel::UUID, std::vector<EventID>>& item) -> uint32_t
				{
					return prior + (uint32_t)item.second.size();
				});
		}

		bool ObjectEventRegistry::Add(Hazel::UUID objectID, EventID eventID)
		{
			std::scoped_lock lock{ m_Mutex };

			if (m_Objects.find(objectID) != m_Objects.end())
			{
				// If object already in the registy, associate eventID with it

				auto& playbacks = m_Objects.at(objectID);

				if (std::find(playbacks.begin(), playbacks.end(), eventID) != playbacks.end())
				{
					ZONG_CORE_ASSERT(false, "EventID already associated to the object.");
					return false;
				}

				playbacks.push_back(eventID);
				return true;
			}
			else
			{
				// Add new object with associated eventID
				return m_Objects.emplace(objectID, std::vector<EventID>{eventID}).second;
			}
		}

		bool ObjectEventRegistry::Remove(Hazel::UUID objectID, EventID eventID)
		{
			std::scoped_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_Objects.find(objectID) != m_Objects.end());

			auto& events = m_Objects.at(objectID);
			auto it = std::find(events.begin(), events.end(), eventID);

			if (it == events.end())
				ZONG_CORE_ERROR("Audio. ObjectEventRegistry. Attempted to remove Event that's not associated to any Object in registry.");
			else
				events.erase(it);

			if (events.empty())
			{
				m_Objects.erase(objectID);
				return true;
			}

			return false;
		}

		bool ObjectEventRegistry::RemoveObject(Hazel::UUID objectID)
		{
			std::scoped_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_Objects.find(objectID) != m_Objects.end());

			return m_Objects.erase(objectID);
		}


		uint32_t ObjectEventRegistry::GetNumberOfEvents(Hazel::UUID objectID) const
		{
			std::shared_lock lock{ m_Mutex };

			if (m_Objects.find(objectID) != m_Objects.end())
				return (uint32_t)m_Objects.at(objectID).size();

			return 0;
		}

		std::vector<EventID> ObjectEventRegistry::GetEvents(Hazel::UUID objectID) const
		{
			std::shared_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_Objects.find(objectID) != m_Objects.end());

			return m_Objects.at(objectID);
		}


		//===============================================================================================

		uint32_t ObjectSourceRegistry::Count() const
		{
			std::shared_lock lock{ m_Mutex };

			return (uint32_t)m_Objects.size();
		}

		uint32_t ObjectSourceRegistry::GetTotalPlaybackCount() const
		{
			std::shared_lock lock{ m_Mutex };

			return (uint32_t)std::accumulate(m_Objects.begin(), m_Objects.end(), 0,
				[](uint32_t prior, const std::pair<Hazel::UUID, std::vector<SourceID>>& item) -> uint32_t
				{
					return prior + (uint32_t)item.second.size();
				});
		}

		bool ObjectSourceRegistry::Add(Hazel::UUID objectID, SourceID sourceID)
		{
			std::scoped_lock lock{ m_Mutex };

			if (m_Objects.find(objectID) != m_Objects.end())
			{
				// If object already in the registy, associate sourceID with it

				auto& sounds = m_Objects.at(objectID);

				if (std::find(sounds.begin(), sounds.end(), sourceID) != sounds.end())
				{
					ZONG_CORE_ASSERT(false, "Active Sound already associated to the object.");
					return false;
				}

				sounds.push_back(sourceID);
				return true;
			}
			else
			{
				// Add new object with associated sourceID
				return m_Objects.emplace(objectID, std::vector<SourceID>{sourceID}).second;
			}
		}

		bool ObjectSourceRegistry::Remove(Hazel::UUID objectID, SourceID sourceID)
		{
			std::scoped_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_Objects.find(objectID) != m_Objects.end());

			auto& sounds = m_Objects.at(objectID);
			auto it = std::find(sounds.begin(), sounds.end(), sourceID);

			if (it == sounds.end())
				ZONG_CORE_ERROR("Audio. ObjectEventRegistry. Attempted to remove Source that's not associated to any Object in registry.");
			else
				sounds.erase(it);

			if (sounds.empty())
			{
				m_Objects.erase(objectID);
				return true;
			}

			return false;
		}

		bool ObjectSourceRegistry::RemoveObject(Hazel::UUID objectID)
		{
			std::scoped_lock lock{ m_Mutex };

			ZONG_CORE_ASSERT(m_Objects.find(objectID) != m_Objects.end());

			return m_Objects.erase(objectID);
		}

		uint32_t ObjectSourceRegistry::GetNumberOfActiveSounds(Hazel::UUID objectID) const
		{
			std::shared_lock lock{ m_Mutex };

			if (m_Objects.find(objectID) != m_Objects.end())
				return (uint32_t)m_Objects.at(objectID).size();

			return 0;
		}

		std::optional<std::vector<SourceID>> ObjectSourceRegistry::GetActiveSounds(Hazel::UUID objectID) const
		{
			std::shared_lock lock{ m_Mutex };

			if (m_Objects.find(objectID) != m_Objects.end())
				return m_Objects.at(objectID);

			return {};
		}

	} // namespace Audio

} // namespace Hazel