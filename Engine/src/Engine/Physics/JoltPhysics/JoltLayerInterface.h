#pragma once

#include "Engine/Core/Base.h"
#include "Engine/Physics/PhysicsLayer.h"

#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace Engine {

	class JoltLayerInterface : public JPH::BroadPhaseLayerInterface
	{
	public:
		JoltLayerInterface();
		virtual ~JoltLayerInterface() = default;

		virtual uint32_t GetNumBroadPhaseLayers() const override { return static_cast<uint32_t>(m_BroadPhaseLayers.size()); }
		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		/// Get the user readable name of a broadphase layer (debugging purposes)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
		{
			const auto& layer = PhysicsLayerManager::GetLayer((uint32_t)((uint8_t)inLayer));
			return layer.Name.c_str();
		}
#endif

	private:
		std::vector<JPH::BroadPhaseLayer> m_BroadPhaseLayers;
	};

}
