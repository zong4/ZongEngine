#pragma once

#include "MeshCookingFactory.h"
#include "PhysicsScene.h"
#include "PhysicsBody.h"
#include "PhysicsCaptureManager.h"

namespace Hazel {

	enum class PhysicsAPIType
	{
		Jolt
	};

	class PhysicsAPI
	{
	public:
		virtual ~PhysicsAPI() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;
		
		virtual Ref<MeshCookingFactory> GetMeshCookingFactory() const = 0;
		virtual Ref<PhysicsScene> CreateScene(const Ref<Scene>& scene) const = 0;
		virtual Ref<PhysicsCaptureManager> GetCaptureManager() const = 0;

		virtual const std::string& GetLastErrorMessage() const = 0;

		static PhysicsAPIType Current() { return s_CurrentPhysicsAPI; }
		static void SetCurrentAPI(PhysicsAPIType api) { s_CurrentPhysicsAPI = api; }

	private:
		inline static PhysicsAPIType s_CurrentPhysicsAPI = PhysicsAPIType::Jolt;
	};

}
