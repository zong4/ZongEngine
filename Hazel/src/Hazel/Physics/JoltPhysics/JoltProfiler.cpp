#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Debug/Profiler.h"

#include <Jolt/Jolt.h>
#include <Jolt/Core/Profiler.h>

#include <thread>

namespace JPH {

#define HZ_ENABLE_JOLT_PROFILING 0

#if HZ_ENABLE_PROFILING && defined(JPH_EXTERNAL_PROFILE) && HZ_ENABLE_JOLT_PROFILING

	static std::mutex s_Mutex;
	static std::unordered_map<std::string_view, Optick::EventDescription*> s_EventDescriptions;

	ExternalProfileMeasurement::ExternalProfileMeasurement(const char* inName, uint32 inColor /* = 0 */)
	{
		memset(mUserData, 0, sizeof(mUserData));

		if (Optick::IsActive(Optick::Mode::DEFAULT) || Optick::IsActive(Optick::Mode::TRACER))
		{
			std::thread::id threadID = std::this_thread::get_id();
			std::string threadName = threadID == Hazel::Application::GetMainThreadID() ? "MainThread" : fmt::format("JoltThread-{}", threadID);
			Optick::ThreadScope threadScope(threadName.c_str());

			s_Mutex.lock();
			Optick::EventDescription*& eventDescription = s_EventDescriptions[inName];
			s_Mutex.unlock();

			if (eventDescription == nullptr)
				eventDescription = Optick::CreateDescription(inName, __FILE__, __LINE__);

			Optick::EventData* eventData = Optick::Event::Start(*eventDescription);
			memcpy(mUserData, &eventData, sizeof(Optick::EventData*));
		}
	}

	ExternalProfileMeasurement::~ExternalProfileMeasurement()
	{
		Optick::EventData** eventData = reinterpret_cast<Optick::EventData**>(mUserData);
		if (eventData != nullptr && *eventData != nullptr)
			Optick::Event::Stop(*(*eventData));
	}

#elif !defined(JPH_PROFILE_ENABLED) && defined(JPH_EXTERNAL_PROFILE)

	ExternalProfileMeasurement::ExternalProfileMeasurement(const char* inName, uint32 inColor /* = 0 */) {}
	ExternalProfileMeasurement::~ExternalProfileMeasurement() {}

#endif

}

