#include <pch.h>
#include "AudioEventsManager.h"

#include "magic_enum.hpp"

#if 0 // Enable logging for Events handling
#define LOG_EVENTS(...) ZONG_CORE_INFO_TAG("Audio", __VA_ARGS__)
#else
#define LOG_EVENTS(...)
#endif

namespace Hazel::Audio {

	AudioEventsManager::AudioEventsManager(MiniAudioEngine& audioEngine)
		: m_AudioEngine(audioEngine)
	{
	}

	bool AudioEventsManager::ProcessTriggerCommand(EventInfo& info)
	{
		bool commandHandled = true;
		
		// Get persistent state object for this Event
		TriggerCommand& trigger = std::get<TriggerCommand>(*info.CommandState);

		//? should this be for the whole Event, or per Action?
		if (trigger.DelayExecution)
			trigger.DelayExecution = false;

		// Action handler dispatch
		auto processPlayAction = [&](TriggerAction& action)
		{
			// Play action.Target on the Context object

			const int sourceID = m_ActionHandler.StartPlayback.Invoke(info.ObjectID, info.EventID, action.Target);

			if (sourceID != Audio::INVALID_SOURCE_ID)
			{
				// TODO: consider moving this source registration to a Delegate of AudioEngine, e.g. OnSouceInit(EventID, SourceID)
				m_EventRegistry.AddSource(info.EventID, sourceID);

				LOG_EVENTS("GameObject: Source PLAY");
				action.Handled = true;
			}
			else
			{
				ZONG_CORE_ERROR_TAG("Audio", "Failed to initialize new Voice for audio object");
				action.Handled = false;
			}
		};
		auto processPauseAction = [&](TriggerAction& action)
		{
			if (action.Context == EActionContext::GameObject)
			{
				// Pause target on the context object

				if (m_ActionHandler.PauseVoicesOnAnObject.Invoke(info.ObjectID, action.Target))
				{
					LOG_EVENTS("GameObject: Source PAUSE");
				}
				else
				{
					ZONG_CORE_ERROR_TAG("Audio", "No Active Sources for object to Pause");
				}
			}
			else // Global context
			{
				// Pause target on all game objects

				m_ActionHandler.PauseVoices.Invoke(action.Target);
				LOG_EVENTS("Global: Source PAUSE");
			}

			action.Handled = true;
		};
		auto processResumeAction = [&](TriggerAction& action)
		{
			if (action.Context == EActionContext::GameObject)
			{
				// Resume target on the context object

				if (m_ActionHandler.ResumeVoicesOnAnObject.Invoke(info.ObjectID, action.Target))
				{
					action.Handled = true;
					LOG_EVENTS("GameObject: Source RESUME");
				}
				else
				{
					//? This is for testing for now, but should probably be handled in Sound object.
					//? In fact, the sound could just reverse fade back to its normal volume level.
					//? Or straight up cancel the pausing.

					// If the sound is Pausing, delay the Resume Action and let its stop-fade to finish
					action.Handled = false;
					commandHandled = false;
					trigger.DelayExecution = true;
				}
			}
			else // Global context
			{
				// Resume target on all Game Objects

				if (m_ActionHandler.ResumeVoices.Invoke(action.Target))
				{
					action.Handled = true;
				}
				else
				{
					//? This is for testing for now, but should probably be handled in Sound object.

					//? This might not work for resuming multiple sources
					action.Handled = false;
					commandHandled = false;
					trigger.DelayExecution = true;
				}

				LOG_EVENTS("Global: Source RESUME");
			}
		};
		auto processStopAction = [&](TriggerAction& action)
		{
			if (action.Context == EActionContext::GameObject)
			{
				// Stop target on the context object

				if (m_ActionHandler.StopVoicesOnAnObject.Invoke(info.ObjectID, action.Target))
				{
					LOG_EVENTS("GameObject: Source STOP");
				}
				else
				{
					ZONG_CORE_ERROR_TAG("Audio", "No Active Sources for object to Stop");
				}
			}
			else // Global context
			{
				// Stop target on all Game Objects

				m_ActionHandler.StopVoices.Invoke(action.Target);
				LOG_EVENTS("Global: Source STOP");
			}

			action.Handled = true;
		};
		auto processStopAllAction = [&](TriggerAction& action)
		{
			if (action.Context == EActionContext::GameObject)
			{
				m_ActionHandler.StopAllOnObject.Invoke(info.ObjectID);
				LOG_EVENTS("GameObject: StopAll");
			}
			else // Global context
			{
				m_ActionHandler.StopAll.Invoke();
				LOG_EVENTS("Global: StopAll");

			}

			action.Handled = true;
		};
		auto processPauseAllAction = [&](TriggerAction& action)
		{
			if (action.Context == EActionContext::GameObject)
			{
				m_ActionHandler.PauseAllOnObject.Invoke(info.ObjectID);
				LOG_EVENTS("GameObject: PauseAll");
			}
			else // Global context
			{
				m_ActionHandler.PauseAll.Invoke();
				LOG_EVENTS("Global: PauseAll");
			}

			action.Handled = true;
		};
		auto processResumeAllAction = [&](TriggerAction& action)
		{
			if (action.Context == EActionContext::GameObject)
			{
				if (m_ActionHandler.ResumeAllOnObject.Invoke(info.ObjectID))
				{
					action.Handled = true;
				}
				else
				{
					//? This is for testing for now, but should probably be handled in Sound object.

						//? This might not work for resuming multiple sources
					action.Handled = false;
					commandHandled = false;
					trigger.DelayExecution = true;
				}

				LOG_EVENTS("GameObject: ResumeAll");
			}
			else // Global context
			{
				if (m_ActionHandler.ResumeAll.Invoke())
				{
					action.Handled = true;
				}
				else
				{
					//? This is for testing for now, but should probably be handled in Sound object.

					//? This might not work for resuming multiple sources
					action.Handled = false;
					commandHandled = false;
					trigger.DelayExecution = true;
				}
				LOG_EVENTS("Global: ResumeAll");
			}
		};

		for (auto& action : trigger.Actions.GetVector())
		{
			if (!action.Target && action.Type != EActionType::StopAll
								&& action.Type != EActionType::PauseAll
								&& action.Type != EActionType::ResumeAll
								&& action.Type != EActionType::SeekAll)
			{
				ZONG_CORE_ERROR_TAG("Audio", "One of the actions of audio trigger ({0}) doesn't have a target assigned!", trigger.DebugName);
				action.Handled = true;
				continue;
			}

			//! in the future migh want some Actions to be able to ignore this flag
			if (trigger.DelayExecution)
				break;

			// If an Action is already handled, this means we have delayed the execution of some other Action of this Event
			// We need to process only unhandled Actions.
			if (action.Handled)
				continue;

			switch (action.Type)
			{
				case EActionType::Play:			processPlayAction(action); break;
				case EActionType::Pause:		processPauseAction(action); break;
				case EActionType::Resume:		processResumeAction(action); break;
				case EActionType::Stop:			processStopAction(action); break;
				case EActionType::StopAll:		processStopAllAction(action); break;
				case EActionType::PauseAll:		processPauseAllAction(action); break;
				case EActionType::ResumeAll:	processResumeAllAction(action); break;
				default:
					action.Handled = true;
					break;
			}

			/* TODO: if action.Handled == false (e.g. delayed execution, or interpolating Action)
						tick timer and, or push back to the queue
				*/
		}

		// Checking if != Play allows to push the Command back to the queue to handle later
		// and prevent calling other Play Actions again if their sources are still playing.
		auto& actions = trigger.Actions.GetVector();
		const bool allActionsHandled = std::all_of(actions.begin(), actions.end(),
													[](const TriggerAction& action)
													{
														// Play action is not handled if failed to initialize source,
														// in which case we're going to clean up this request from the registries.
														// TODO: in the future maybe fix/reverse this logic, but the event needs
														//		to be active while the sound is playing.
														if (action.Type == EActionType::Play)
															return !action.Handled;
														else
															return action.Handled;
													});

		// If all Actions have been handled, remove Event from the registry
		if (allActionsHandled)
		{
			m_EventRegistry.Remove(info.EventID);
			m_OnEventFinished.Invoke(info.EventID, info.ObjectID);
		}

		return commandHandled;
	}

	bool AudioEventsManager::ProcessSwitchCommand(EventInfo& info)
	{
		ZONG_CORE_ASSERT(false, "Not implemented.");
		return false;
	}

	bool AudioEventsManager::ProcessStateCommand(EventInfo& info)
	{
		ZONG_CORE_ASSERT(false, "Not implemented.");
		return false;
	}

	bool AudioEventsManager::ProcessParameterCommand(EventInfo& info)
	{
		ZONG_CORE_ASSERT(false, "Not implemented.");
		return false;
	}

	void AudioEventsManager::Update(Timestep ts)
	{
		//---  Execute queued commands ---
		//--------------------------------

		for (int i = (int)m_CommandQueue.size() - 1; i >= 0; i--)
		{

			// Flag to add a Command back to the queue and process/complete later
			// Play Action is exception, because it's handled when the source playback is complete.
			const bool commandHandled = [&]
			{
				auto& [type, info] = m_CommandQueue.front();
				
				switch (type)
				{
					case ECommandType::Trigger: return ProcessTriggerCommand(info);
					case ECommandType::Switch: return ProcessSwitchCommand(info);
					case ECommandType::State: return ProcessStateCommand(info);
					case ECommandType::Parameter: return ProcessParameterCommand(info);
					default:
						return true;
				}
			}();

			// TODO: Events might require timestep for interpolation of some kind,
			//       or some other time tracking mechanism.
			if (!commandHandled)
				m_CommandQueue.push(m_CommandQueue.front());

			m_CommandQueue.pop();
		}
	}

	EventID AudioEventsManager::RegisterEvent(EventInfo& info)
	{
		if (!info.CommandState)
			return EventID::INVALID;

		return m_EventRegistry.Add(info);
	}

	void AudioEventsManager::OnSourceFinished(EventID playingEvent, SourceID source)
	{
		if (m_EventRegistry.RemoveSource(playingEvent, source))
		{
			// If all Actions of the Event were handled (e.g. this Play Action was the last one),
			// remove Event from the object registry.

			auto info = m_EventRegistry.Get(playingEvent);
			TriggerCommand& trigger = std::get<TriggerCommand>(*info.CommandState);
			auto& actions = trigger.Actions.GetVector();
			const bool allActionsHandled = std::all_of(actions.begin(), actions.end(),
										[](const TriggerAction& action) {return action.Handled; });

			if (allActionsHandled)
			{
				m_EventRegistry.Remove(playingEvent);
				m_OnEventFinished.Invoke(playingEvent, info.ObjectID);
			}
		}
	}

	void AudioEventsManager::PostTrigger(const EventInfo& eventInfo)
	{
		if (!eventInfo.CommandState)
			return;

		// Push command to the queue.
		m_CommandQueue.push({ ECommandType::Trigger, eventInfo }); // TODO: alternative option is to push playbackID and retrieve EventInfo in the Update()
	}

	bool AudioEventsManager::ExecuteActionOnPlayingEvent(EventID playingEvent, EActionTypeOnPlayingEvent action)
	{
		if (m_EventRegistry.GetNumberOfSources(playingEvent) <= 0)
		{
			ZONG_CONSOLE_LOG_WARN("Audio. Trying to execute action {} on playing event that's not in the active events registry.", magic_enum::enum_name(action));
			return false;
		}

		auto executeAction = [=]
		{
			const EventInfo eventInfo = m_EventRegistry.Get(playingEvent);
			m_ActionHandler.ExecuteOnSources.Invoke(action, eventInfo.ActiveSources);
		};

		MiniAudioEngine::ExecuteOnAudioThread(MiniAudioEngine::EIfThisIsAudioThread::ExecuteNow, executeAction, "ExecuteActionOnPlayingEvent");

		return true;
	}

} // namespace Hazel::Audio
