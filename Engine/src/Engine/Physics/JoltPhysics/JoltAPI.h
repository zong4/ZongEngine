#pragma once

#include "Engine/Physics/PhysicsAPI.h"

#ifdef HZ_ENABLE_ASSERTS
#define JPH_ENABLE_ASSERTS
#endif

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/Profiler.h>

namespace Hazel {

	class JoltAPI : public PhysicsAPI
	{
	public:
		JoltAPI();
		~JoltAPI();

		virtual void Init() override;
		virtual void Shutdown() override;

		JPH::TempAllocator* GetTempAllocator() const;
		JPH::JobSystemThreadPool* GetJobThreadPool() const;

		virtual const std::string& GetLastErrorMessage() const override;
		virtual Ref<PhysicsScene> CreateScene(const Ref<Scene>& scene) const override;

		virtual Ref<MeshCookingFactory> GetMeshCookingFactory() const override;
		virtual Ref<PhysicsCaptureManager> GetCaptureManager() const override;
	};

}
