#include "hzpch.h"
#include "JoltAPI.h"

#include "JoltScene.h"
#include "JoltCookingFactory.h"
#include "JoltCaptureManager.h"

#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

namespace Hazel {

	struct JoltData
	{
		JPH::TempAllocator* TemporariesAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> JobThreadPool;

		std::string LastErrorMessage = "";

		Ref<JoltCookingFactory> CookingFactory = nullptr;
		Ref<JoltCaptureManager> CaptureManager = nullptr;
	};

	static JoltData* s_JoltData = nullptr;

	static void JoltTraceCallback(const char* format, ...)
	{
		va_list list;
		va_start(list, format);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), format, list);

		if (s_JoltData)
		{
			s_JoltData->LastErrorMessage = buffer;
		}
		HZ_CORE_TRACE_TAG("Physics", buffer);
	}

#ifdef JPH_ENABLE_ASSERTS

	static bool JoltAssertFailedCallback(const char* expression, const char* message, const char* file, uint32_t line)
	{
		HZ_CORE_FATAL_TAG("Physics", "{}:{}: ({}) {}", file, line, expression, message != nullptr ? message : "");
		return true;
	}

#endif

	JoltAPI::JoltAPI()
	{

	}

	JoltAPI::~JoltAPI()
	{

	}

#define HZ_ENABLE_MALLOC_ALLOC 0

	void JoltAPI::Init()
	{
		HZ_CORE_VERIFY(!s_JoltData, "Can't initialize Jolt multiple times!");

		JPH::RegisterDefaultAllocator();

		JPH::Trace = JoltTraceCallback;

#ifdef JPH_ENABLE_ASSERTS
		JPH::AssertFailed = JoltAssertFailedCallback;
#endif

		JPH::Factory::sInstance = new JPH::Factory();

		JPH::RegisterTypes();

		s_JoltData = hnew JoltData();

#if HZ_ENABLE_MALLOC_ALLOC
		// NOTE(Peter): We shouldn't be using this allocator if we want the best performance
		s_JoltData->TemporariesAllocator = new JPH::TempAllocatorMalloc();
#else
		s_JoltData->TemporariesAllocator = new JPH::TempAllocatorImpl(300 * 1024 * 1024); // 10 mb
#endif

		// NOTE(Peter): Just construct the thread pool for now, don't start the threads
		s_JoltData->JobThreadPool = std::make_unique<JPH::JobSystemThreadPool>(2048, 8, 6);

		s_JoltData->CookingFactory = Ref<JoltCookingFactory>::Create();
		s_JoltData->CookingFactory->Init();

		s_JoltData->CaptureManager = Ref<JoltCaptureManager>::Create();
	}

	void JoltAPI::Shutdown()
	{
		s_JoltData->CaptureManager = nullptr;

		s_JoltData->CookingFactory->Shutdown();
		s_JoltData->CookingFactory = nullptr;

		delete s_JoltData->TemporariesAllocator;

		hdelete s_JoltData;
		s_JoltData = nullptr;

		delete JPH::Factory::sInstance;
	}

	JPH::TempAllocator* JoltAPI::GetTempAllocator() const { return s_JoltData->TemporariesAllocator; }
	JPH::JobSystemThreadPool* JoltAPI::GetJobThreadPool() const { return s_JoltData->JobThreadPool.get(); }

	const std::string& JoltAPI::GetLastErrorMessage() const { return s_JoltData->LastErrorMessage; }

	Ref<PhysicsScene> JoltAPI::CreateScene(const Ref<Scene>& scene) const { return Ref<JoltScene>::Create(scene); }

	Ref<MeshCookingFactory> JoltAPI::GetMeshCookingFactory() const { return s_JoltData->CookingFactory; }

	Ref<PhysicsCaptureManager> JoltAPI::GetCaptureManager() const { return s_JoltData->CaptureManager; }

}
