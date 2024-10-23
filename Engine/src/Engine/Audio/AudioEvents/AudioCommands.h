#pragma once

#include "Engine/Audio/Sound.h"
#include "Engine/Audio/Editor/OrderedVector.h"

namespace Hazel::Audio
{
	enum EActionType : int
	{
		Play, Stop, StopAll, Pause, PauseAll, Resume, ResumeAll, Break, Seek, SeekAll, PostTrigger, ReleaseEnvelope
	};

	enum class EActionTypeOnPlayingEvent : int
	{
		Stop, Pause, Resume, Break, ReleaseEnvelope
	};

	enum EActionContext : int
	{
		GameObject, Global
	};


	enum class ECommandType
	{
		Trigger, Switch, State, Parameter
	};

	struct TriggerAction
	{
		EActionType Type;
		Ref<SoundConfig> Target;
		EActionContext Context;

		bool Handled = false;

		bool operator==(const TriggerAction& other) const
		{
			return Type == other.Type && Target == other.Target && Context == other.Context && Handled == other.Handled;
		}
	};

	struct CommandBase
	{
		std::string DebugName;
		bool Handled = false;
		virtual ECommandType GetType() const = 0;
	};

	struct TriggerCommand : public CommandBase
	{
		virtual ECommandType GetType() const override { return ECommandType::Trigger; }
		Hazel::OrderedVector<TriggerAction> Actions;

		/* This is used when need to skip executing the Event to the next Update, e.g. waiting for "stop-fade"
		   It might be useful to have in a base class, or in some other, more generic form. */
		bool DelayExecution = false; 

		TriggerCommand& operator=(const TriggerCommand& other)
		{
			if (this == &other)
				return *this;

			Actions.SetValues(other.Actions.GetVector());
			DebugName = other.DebugName;
			return *this;
		}

	};

	struct SwitchCommand : public CommandBase
	{
		virtual ECommandType GetType() const override { return ECommandType::Switch; }
	};

	struct StateCommand : public CommandBase
	{
		virtual ECommandType GetType() const override { return ECommandType::State; }
	};

	struct ParameterCommand: public CommandBase
	{
		virtual ECommandType GetType() const override { return ECommandType::Parameter; }
	};



} // namespace Hazel::Audio

namespace Hazel::Utils {

	inline Audio::EActionType AudioActionTypeFromString(const std::string& actionType)
	{
		if (actionType == "Play")		return Audio::EActionType::Play;
		if (actionType == "Stop")		return Audio::EActionType::Stop;
		if (actionType == "StopAll")	return Audio::EActionType::StopAll;
		if (actionType == "Pause")		return Audio::EActionType::Pause;
		if (actionType == "PauseAll")	return Audio::EActionType::PauseAll;
		if (actionType == "Resume")		return Audio::EActionType::Resume;
		if (actionType == "ResumeAll")	return Audio::EActionType::ResumeAll;
		if (actionType == "Break")		return Audio::EActionType::Break;
		if (actionType == "Seek")		return Audio::EActionType::Seek;
		if (actionType == "SeekAll")	return Audio::EActionType::SeekAll;
		if (actionType == "PostTrigger")return Audio::EActionType::PostTrigger;
		
		HZ_CORE_ASSERT(false, "Unknown Action Type");
		return Audio::EActionType::Play;
	}

	inline const char* AudioActionTypeToString(Audio::EActionType actionType)
	{
		switch (actionType)
		{
		case Audio::EActionType::Play:			return "Play";
		case Audio::EActionType::Stop:			return "Stop";
		case Audio::EActionType::StopAll:		return "StopAll";
		case Audio::EActionType::Pause:			return "Pause";
		case Audio::EActionType::PauseAll:		return "PauseAll";
		case Audio::EActionType::Resume:		return "Resume";
		case Audio::EActionType::ResumeAll:		return "ResumeAll";
		case Audio::EActionType::Break:			return "Break";
		case Audio::EActionType::Seek:			return "Seek";
		case Audio::EActionType::SeekAll:		return "SeekAll";
		case Audio::EActionType::PostTrigger:	return "PostTrigger";
		}

		HZ_CORE_ASSERT(false, "Unknown Action Type");
		return "None";
	}

	inline Audio::EActionContext AudioActionContextFromString(const std::string& context)
	{
		if (context == "GameObject")	return Audio::EActionContext::GameObject;
		if (context == "Global")		return Audio::EActionContext::Global;

		HZ_CORE_ASSERT(false, "Unknown Context Type");
		return Audio::EActionContext::GameObject;
	}

	inline const char* AudioActionContextToString(Audio::EActionContext context)
	{
		switch (context)
		{
		case Audio::EActionContext::GameObject:	return "GameObject";
		case Audio::EActionContext::Global:		return "Global";
		}

		HZ_CORE_ASSERT(false, "Unknown Context Type");
		return "None";
	}

} // namespace Hazel::Utils
