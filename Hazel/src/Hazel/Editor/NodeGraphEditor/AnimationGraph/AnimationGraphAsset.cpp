#include <hzpch.h>
#include "AnimationGraphAsset.h"

#include "AnimationGraphEditorTypes.h"
#include "Hazel/Animation/AnimationGraphFactory.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Utilities/ContainerUtils.h"

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

		auto appendAnimationAsset = [&animationHandles](const choc::value::ValueView& value)
		{
			AssetHandle assetHandle = Utils::GetAssetHandleFromValue(value);
			if (assetHandle != 0)
			{
				Utils::AppendIfNotPresent(animationHandles, assetHandle);
			}
		};

		for (const auto& [subGraphId, nodes] : m_Nodes)
		{
			for (const auto& node : nodes)
			{
				for (const auto& input : node->Inputs)
				{
					if (input->GetType() != AG::Types::EPinType::AnimationAsset || isPinLinked(input->ID))
						continue;

					appendAnimationAsset(input->Value);
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
				{
					appendAnimationAsset(animation);
				}
			}
			else if (Utils::IsAssetHandle<AssetType::Animation>(value))
			{
				appendAnimationAsset(value);
			}
		}

		for (const auto& localVariableName : LocalVariables.GetNames())
		{
			choc::value::Value value = LocalVariables.GetValue(localVariableName);
			const bool isArray = value.isArray();

			if (isArray && Utils::IsAssetHandle<AssetType::Animation>(value[0]))
			{
				for (const auto& animation : value)
				{
					appendAnimationAsset(animation);
				}
			}
			else if (Utils::IsAssetHandle<AssetType::Animation>(value))
			{
				appendAnimationAsset(value);
			}
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
