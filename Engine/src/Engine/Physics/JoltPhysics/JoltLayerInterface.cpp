#include "pch.h"
#include "JoltLayerInterface.h"

namespace Engine {

	JoltLayerInterface::JoltLayerInterface()
	{
		uint32_t layerCount = PhysicsLayerManager::GetLayerCount();
		m_BroadPhaseLayers.resize(layerCount);

		for (const auto& layer : PhysicsLayerManager::GetLayers())
			m_BroadPhaseLayers[layer.LayerID] = JPH::BroadPhaseLayer(layer.LayerID);
	}

	JPH::BroadPhaseLayer JoltLayerInterface::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
	{
		ZONG_CORE_VERIFY(inLayer < m_BroadPhaseLayers.size());
		return m_BroadPhaseLayers[inLayer];
	}

}
