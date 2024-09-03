#include "hzpch.h"
#include "PhysicsLayer.h"
#include "Hazel/Utilities/ContainerUtils.h"

namespace Hazel {

	uint32_t PhysicsLayerManager::AddLayer(const std::string& name, bool setCollisions)
	{
		for (const auto& layer : s_Layers)
		{
			if (layer.Name == name)
				return layer.LayerID;
		}

		uint32_t layerId = GetNextLayerID();
		PhysicsLayer layer = { layerId, name, static_cast<int32_t>(BIT(layerId)), static_cast<int32_t>(BIT(layerId)) };
		s_Layers.insert(s_Layers.begin() + layerId, layer);
		s_LayerNames.insert(s_LayerNames.begin() + layerId, name);

		if (setCollisions)
		{
			for (const auto& layer2 : s_Layers)
			{
				SetLayerCollision(layer.LayerID, layer2.LayerID, true);
			}
		}

		return layer.LayerID;
	}

	void PhysicsLayerManager::RemoveLayer(uint32_t layerId)
	{
		PhysicsLayer& layerInfo = GetLayer(layerId);

		for (auto& otherLayer : s_Layers)
		{
			if (otherLayer.LayerID == layerId)
				continue;

			if (otherLayer.CollidesWith & layerInfo.BitValue)
			{
				otherLayer.CollidesWith &= ~layerInfo.BitValue;
			}
		}

		Utils::RemoveIf(s_LayerNames, [&](const std::string& name) { return name == layerInfo.Name; });
		Utils::RemoveIf(s_Layers, [&](const PhysicsLayer& layer) { return layer.LayerID == layerId; });
	}

	void PhysicsLayerManager::UpdateLayerName(uint32_t layerId, const std::string& newName)
	{
		for (const auto& layerName : s_LayerNames)
		{
			if (layerName == newName)
				return;
		}

		PhysicsLayer& layer = GetLayer(layerId);
		Utils::RemoveIf(s_LayerNames, [&](const std::string& name) { return name == layer.Name; });
		s_LayerNames.insert(s_LayerNames.begin() + layerId, newName);
		layer.Name = newName;
	}

	void PhysicsLayerManager::SetLayerCollision(uint32_t layerId, uint32_t otherLayer, bool shouldCollide)
	{
		if (ShouldCollide(layerId, otherLayer) && shouldCollide)
			return;

		PhysicsLayer& layerInfo = GetLayer(layerId);
		PhysicsLayer& otherLayerInfo = GetLayer(otherLayer);

		if (shouldCollide)
		{
			layerInfo.CollidesWith |= otherLayerInfo.BitValue;
			otherLayerInfo.CollidesWith |= layerInfo.BitValue;
		}
		else
		{
			layerInfo.CollidesWith &= ~otherLayerInfo.BitValue;
			otherLayerInfo.CollidesWith &= ~layerInfo.BitValue;
		}
	}

	std::vector<PhysicsLayer> PhysicsLayerManager::GetLayerCollisions(uint32_t layerId)
	{
		const PhysicsLayer& layer = GetLayer(layerId);

		std::vector<PhysicsLayer> layers;
		for (const auto& otherLayer : s_Layers)
		{
			if (otherLayer.LayerID == layerId)
				continue;

			if (layer.CollidesWith & otherLayer.BitValue)
				layers.push_back(otherLayer);
		}

		return layers;
	}

	PhysicsLayer& PhysicsLayerManager::GetLayer(uint32_t layerId)
	{
		return layerId >= s_Layers.size() ? s_NullLayer : s_Layers[layerId];
	}

	PhysicsLayer& PhysicsLayerManager::GetLayer(const std::string& layerName)
	{
		for (auto& layer : s_Layers)
		{
			if (layer.Name == layerName)
				return layer;
		}

		return s_NullLayer;
	}

	bool PhysicsLayerManager::ShouldCollide(uint32_t layer1, uint32_t layer2)
	{
		return GetLayer(layer1).CollidesWith & GetLayer(layer2).BitValue;
	}

	bool PhysicsLayerManager::IsLayerValid(uint32_t layerId)
	{
		const PhysicsLayer& layer = GetLayer(layerId);
		return layer.LayerID != s_NullLayer.LayerID && layer.IsValid();
	}

	bool PhysicsLayerManager::IsLayerValid(const std::string& layerName)
	{
		const PhysicsLayer& layer = GetLayer(layerName);
		return layer.LayerID != s_NullLayer.LayerID && layer.IsValid();
	}

	void PhysicsLayerManager::ClearLayers()
	{
		s_Layers.clear();
		s_LayerNames.clear();
	}

	uint32_t PhysicsLayerManager::GetNextLayerID()
	{
		int32_t lastId = -1;

		for (const auto& layer : s_Layers)
		{
			if (lastId != -1 && int32_t(layer.LayerID) != lastId + 1)
				return uint32_t(lastId + 1);

			lastId = layer.LayerID;
		}

		return (uint32_t)s_Layers.size();
	}

	std::vector<PhysicsLayer> PhysicsLayerManager::s_Layers;
	std::vector<std::string> PhysicsLayerManager::s_LayerNames;
	PhysicsLayer PhysicsLayerManager::s_NullLayer = { (uint32_t)- 1, "NULL", -1, -1};

}
