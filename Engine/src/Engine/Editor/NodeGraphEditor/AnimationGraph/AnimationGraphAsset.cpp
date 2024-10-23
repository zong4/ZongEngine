#include <hzpch.h>
#include "AnimationGraphAsset.h"

#include "AnimationGraphEditorTypes.h"
#include "Engine/Animation/AnimationGraphFactory.h"
#include "Engine/Asset/AssetManager.h"
#include "Engine/Utilities/ContainerUtils.h"

namespace AG = Hazel::AnimationGraph;

namespace Hazel {

	AssetHandle AnimationGraphAsset::GetSkeletonHandle() const
	{
		return m_SkeletonHandle;
	}


	std::vector<AssetHandle> AnimationGraphAsset::GetAnimationHandles() const
	{
		std::vector<AssetHandle> animationHandles;

		// TODO (0x): this doesn't belong here, but I don't want to pull in AnimationGraphEditorModel.
		auto isPinLinked = [this](UUID id)
		{
			if (!id) return false;

			for (const auto& [subGraphId, links] : m_Links)
			{
				for (const auto& link : links)
				{
					if (link.StartPinID == id || link.EndPinID == id) return true;
				}
			}

			return false;
		};


		for (const auto& [subGraphId, nodes] : m_Nodes)
		{
			for (const auto& node : nodes)
			{
				for (const auto& input : node->Inputs)
				{
					if (input->GetType() != AG::Types::EPinType::AnimationAsset || isPinLinked(input->ID))
						continue;

					if (input->Value.isInt64())
					{
						AssetHandle assetHandle = input->Value.getInt64();
						if (assetHandle != 0)
						{
							Utils::AppendIfNotPresent(animationHandles, assetHandle);
						}
					}
				}
			}
		}

		for (const auto& inputName : Inputs.GetNames())
		{
			choc::value::ValueView value = Inputs.GetValue(inputName);
			const bool isArray = value.isArray();

			if (isArray && Utils::IsAssetHandle<AssetType::Animation>(value[0]))
			{
				for (const auto& animation : value)
					Utils::AppendIfNotPresent(animationHandles, (AssetHandle)animation.getObjectMemberAt(0).value.getInt64());
			}
			else if (Utils::IsAssetHandle<AssetType::Animation>(value))
				Utils::AppendIfNotPresent(animationHandles, (AssetHandle)value.getObjectMemberAt(0).value.getInt64());
		}

		for (const auto& localVariableName : LocalVariables.GetNames())
		{
			choc::value::Value value = LocalVariables.GetValue(localVariableName);
			const bool isArray = value.isArray();

			if (isArray && Utils::IsAssetHandle<AssetType::Animation>(value[0]))
			{
				for (const auto& animation : value)
					Utils::AppendIfNotPresent(animationHandles, (AssetHandle)animation.getObjectMemberAt(0).value.getInt64());
			}
			else if (Utils::IsAssetHandle<AssetType::Animation>(value))
				Utils::AppendIfNotPresent(animationHandles, (AssetHandle)value.getObjectMemberAt(0).value.getInt64());
		}

		return animationHandles;
	}


	Ref<AnimationGraph::AnimationGraph> AnimationGraphAsset::CreateInstance()
	{
		Ref<AnimationGraph::AnimationGraph> instance;
		if (Prototype)
		{
			instance = AnimationGraph::CreateInstance(*Prototype);
			if (instance)
			{
				instance->Init();
			}
		}
		return instance;
	}

}
