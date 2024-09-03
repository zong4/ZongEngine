#pragma once

#include "Hazel/Asset/Asset.h"

#include "Hazel/Animation/AnimationGraph.h"
#include "Hazel/Animation/AnimationGraphPrototype.h"

#include "Hazel/Editor/NodeGraphEditor/Nodes.h"
#include "Hazel/Editor/NodeGraphEditor/PropertySet.h"

namespace Hazel {

	// Wraps AnimationGraph::Prototype as an Asset
	// This asset can be used to create instances of AnimationGraph (from the Prototype)
	//
	// For now this also contains the editor representation of the animation graph.
	// Technically only the prototype is needed for the runtime.
	class AnimationGraphAsset : public Asset
	{
	public:
		virtual ~AnimationGraphAsset() = default;

		static AssetType GetStaticType() { return AssetType::AnimationGraph; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		// Get the skeleton (asset) that this animation graph animates
		AssetHandle GetSkeletonHandle() const;

		// Get the animation assets referenced by this animation graph
		std::vector<AssetHandle> GetAnimationHandles() const;

		Scope<AnimationGraph::Prototype> Prototype;

		Ref<AnimationGraph::AnimationGraph> CreateInstance();

		Utils::PropertySet Inputs;
		Utils::PropertySet Outputs;
		Utils::PropertySet LocalVariables;

	private:
		AssetHandle m_SkeletonHandle = 0;
		std::unordered_map<UUID, std::string> m_GraphState = { {0, {}} }; //? Should not have this in Runtime asset
		std::unordered_map<UUID, std::vector<Node*>> m_Nodes = { {0, {}} };
		std::unordered_map<UUID, std::vector<Link>> m_Links = { {0, {}} };

		friend class AnimationGraphNodeEditorModel;
		friend std::string SerializeToYAML(Ref<AnimationGraphAsset>);
		friend bool DeserializeFromYAML(const std::string&, Ref<AnimationGraphAsset>&);
	};

} // namespace Hazel
