#include "pch.h"
#include "AnimationAssetSerializer.h"

#include "GraphSerializer.h"
#include "Engine/Animation/Animation.h"
#include "Engine/Animation/AnimationGraph.h"
#include "Engine/Animation/Skeleton.h"
#include "Engine/Asset/AssetManager.h"
#include "Engine/Editor/NodeGraphEditor/AnimationGraph/AnimationGraphAsset.h"            // }- TODO (0x): separate editor from runtime
#include "Engine/Editor/NodeGraphEditor/AnimationGraph/AnimationGraphNodeEditorModel.h"  // }
#include "Engine/Renderer/Mesh.h"
#include "Engine/Serialization/FileStream.h"
#include "Engine/Utilities/SerializationMacros.h"
#include "Engine/Utilities/YAMLSerializationHelpers.h"

#include <choc/text/choc_JSON.h>
#include <yaml-cpp/yaml.h>

namespace AG = Hazel::AnimationGraph;


namespace Hazel {

	//////////////////////////////////////////////////////////////////////////////////
	// SkeletonAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	std::string SerializeToYAML(Ref<SkeletonAsset> skeleton)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		{
			out << YAML::Key << "Skeleton" << YAML::Value;
			out << YAML::BeginMap;
			{
				out << YAML::Key << "MeshSource" << YAML::Value << skeleton->GetMeshSource()->Handle;
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		return std::string(out.c_str());
	}


	bool DeserializeFromYAML(const std::string& yamlString, Ref<SkeletonAsset>& skeleton)
	{
		YAML::Node data = YAML::Load(yamlString);
		if (!data["Skeleton"])
			return false;

		YAML::Node rootNode = data["Skeleton"];
		if (!rootNode["MeshSource"])
			return false;

		AssetHandle meshSourceHandle = 0;
		meshSourceHandle = rootNode["MeshSource"].as<uint64_t>(meshSourceHandle);

		if (!AssetManager::IsAssetHandleValid(meshSourceHandle))
			return false;

		skeleton = Ref<SkeletonAsset>::Create(meshSourceHandle);
		return true;
	}


	void SkeletonAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<SkeletonAsset> skeleton = asset.As<SkeletonAsset>();

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		ZONG_CORE_VERIFY(fout.good());
		std::string yamlString = SerializeToYAML(skeleton);
		fout << yamlString;
	}


	bool SkeletonAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto filepath = Project::GetAssetDirectory() / metadata.FilePath;
		std::ifstream stream(filepath);
		ZONG_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<SkeletonAsset> skeleton;
		bool result = DeserializeFromYAML(strStream.str(), skeleton);
		if (!result)
			return false;

		asset = skeleton;
		asset->Handle = metadata.Handle;
		return true;
	}


	bool SkeletonAssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<SkeletonAsset> skeleton = AssetManager::GetAsset<SkeletonAsset>(handle);
		std::string yamlString = SerializeToYAML(skeleton);

		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}


	Ref<Asset> SkeletonAssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<SkeletonAsset> skeletonAsset;
		bool result = DeserializeFromYAML(yamlString, skeletonAsset);
		if (!result)
			return nullptr;

		return skeletonAsset;
	}


	//////////////////////////////////////////////////////////////////////////////////
	// AnimationAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	std::string SerializeToYAML(Ref<AnimationAsset> animationAsset)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		{
			out << YAML::Key << "Animation" << YAML::Value;
			out << YAML::BeginMap;
			{
				out << YAML::Key << "AnimationSource" << YAML::Value << animationAsset->GetAnimationSource()->Handle;
				out << YAML::Key << "SkeletonSource" << YAML::Value << animationAsset->GetSkeletonSource()->Handle;
				out << YAML::Key << "AnimationIndex" << YAML::Value << animationAsset->GetAnimationIndex();
				out << YAML::Key << "FilterRootMotion" << YAML::Value << animationAsset->IsMaskedRootMotion();
				out << YAML::Key << "RootTranslationMask" << YAML::Value << animationAsset->GetRootTranslationMask();
				out << YAML::Key << "RootRotationMask" << YAML::Value << animationAsset->GetRootTranslationMask();
			}
			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		return std::string(out.c_str());
	}


	bool DeserializeFromYAML(const std::string& yamlString, Ref<AnimationAsset>& animationAsset)
	{
		YAML::Node data = YAML::Load(yamlString);
		if (!data["Animation"])
			return false;

		YAML::Node rootNode = data["Animation"];
		if (!rootNode["MeshSource"] && (!rootNode["AnimationSource"] || !rootNode["SkeletonSource"]))
			return false;

		AssetHandle animationHandle = 0;
		AssetHandle skeletonHandle = 0;
		if (rootNode["MeshSource"])
		{
			animationHandle = rootNode["MeshSource"].as<uint64_t>(animationHandle);
			skeletonHandle = animationHandle;
		}
		else
		{
			animationHandle = rootNode["AnimationSource"].as<uint64_t>(animationHandle);
			skeletonHandle = rootNode["SkeletonSource"].as<uint64_t>(skeletonHandle);
		}

		Ref<MeshSource> animationSource = AssetManager::GetAsset<MeshSource>(animationHandle);
		Ref<MeshSource> skeletonSource = AssetManager::GetAsset<MeshSource>(skeletonHandle);

		if (!animationSource || !animationSource->IsValid() || !skeletonSource || !skeletonSource->IsValid())
			return false;

		uint32_t animationIndex = 0;
		animationIndex = rootNode["AnimationIndex"].as<uint32_t>(animationIndex);
		auto filterRootMotion = rootNode["FilterRootMotion"].as<bool>(true);
		auto rootTranslationMask = rootNode["RootTranslationMask"].as<glm::vec3>(glm::vec3{ 0.0f });
		auto rootRotationMask = rootNode["RootRotationMask"].as<float>(0.0f);

		animationAsset = Ref<AnimationAsset>::Create(animationSource, skeletonSource, animationIndex, filterRootMotion, rootTranslationMask, rootRotationMask);
		return true;
	}


	void AnimationAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<AnimationAsset> animationAsset = asset.As<AnimationAsset>();

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		ZONG_CORE_VERIFY(fout.good());
		std::string yamlString = SerializeToYAML(animationAsset);
		fout << yamlString;
	}


	bool AnimationAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		auto filepath = Project::GetAssetDirectory() / metadata.FilePath;
		std::ifstream stream(filepath);
		ZONG_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<AnimationAsset> animationAsset;
		bool result = DeserializeFromYAML(strStream.str(), animationAsset);
		if (!result)
			return false;

		asset = animationAsset;
		asset->Handle = metadata.Handle;
		return true;
	}


	bool AnimationAssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<AnimationAsset> animationAsset = AssetManager::GetAsset<AnimationAsset>(handle);
		std::string yamlString = SerializeToYAML(animationAsset);

		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}


	Ref<Asset> AnimationAssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<AnimationAsset> animationAsset;
		bool result = DeserializeFromYAML(yamlString, animationAsset);
		if (!result)
			return nullptr;

		return animationAsset;
	}


	//////////////////////////////////////////////////////////////////////////////////
	// AnimationGraphAssetSerializer
	//////////////////////////////////////////////////////////////////////////////////

	std::string SerializeToYAML(Ref<AnimationGraphAsset> graph)
	{
		// Create an ordered set of ids.
		// We will serialize the nodes in order of id (so that we do not get spurious changes in the serialized asset due to ordering)
		std::set<UUID> ids;
		for (const auto& [id, nodes] : graph->m_Nodes)
		{
			ids.insert(id);
		}

		YAML::Emitter out;
		out << YAML::BeginMap;
		{
			ZONG_SERIALIZE_PROPERTY(Skeleton, graph->m_SkeletonHandle, out);
			for(const auto id : ids) {
				const auto& nodes = graph->m_Nodes.at(id);
				out << YAML::Key << id << YAML::Value;
				out << YAML::BeginMap;
				{
					DefaultGraphSerializer::SerializeNodes(out, nodes);
					DefaultGraphSerializer::SerializeLinks(out, graph->m_Links.at(id));
					ZONG_SERIALIZE_PROPERTY(GraphState, graph->m_GraphState.at(id), out);
				}
				out << YAML::EndMap;
			}
			ZONG_SERIALIZE_PROPERTY(GraphInputs, graph->Inputs.ToExternalValue(), out);
			ZONG_SERIALIZE_PROPERTY(GraphOutputs, graph->Outputs.ToExternalValue(), out);
			ZONG_SERIALIZE_PROPERTY(LocalVariables, graph->LocalVariables.ToExternalValue(), out);
		}
		out << YAML::EndMap;

		return std::string(out.c_str());
	}


	bool DeserializeFromYAML(const std::string& yamlString, Ref<AnimationGraphAsset>& graph)
	{
		YAML::Node data;

		try
		{
			data = YAML::Load(yamlString);
		}
		catch (const std::exception& e)
		{
			ZONG_CONSOLE_LOG_FATAL(e.what());
			return false;
		}

		graph = Ref<AnimationGraphAsset>::Create();

		// Graph IO (must be deserialized before nodes to be able to spawn IO nodes)
		{
			choc::value::Value graphInputsValue;
			choc::value::Value graphOutputsValue;
			choc::value::Value graphLocalVariablesValue;

			ZONG_DESERIALIZE_PROPERTY(Skeleton, graph->m_SkeletonHandle, data, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(GraphInputs, graphInputsValue, data, choc::value::Value());
			ZONG_DESERIALIZE_PROPERTY(GraphOutputs, graphOutputsValue, data, choc::value::Value());
			ZONG_DESERIALIZE_PROPERTY(LocalVariables, graphLocalVariablesValue, data, choc::value::Value());

			graph->Inputs = Utils::PropertySet::FromExternalValue(graphInputsValue);
			graph->Outputs = Utils::PropertySet::FromExternalValue(graphOutputsValue);
			graph->LocalVariables = Utils::PropertySet::FromExternalValue(graphLocalVariablesValue);
		}

		using PinCandidate = DefaultGraphSerializer::DeserializationFactory::PinCandidate;
		using NodeCandidate = DefaultGraphSerializer::DeserializationFactory::NodeCandidate;

		// Nodes
		auto constructNode = [&graph](const NodeCandidate& candidate, const std::optional<std::vector<PinCandidate>>& inputs, const std::optional<std::vector<PinCandidate>>& outputs)
		{
			// graph IO, Local Variables
			auto optType = magic_enum::enum_cast<EPropertyType>(choc::text::replace(candidate.Category, " ", ""));

			Node* newNode = nullptr;

			if (optType.has_value())
			{
				auto getVariableName = [&]()
				{
					ZONG_CORE_ASSERT(inputs.has_value() || outputs.has_value())
					std::string name;
					if (outputs && !outputs->empty())
						name = outputs->at(0).Name;
					else if (inputs && !inputs->empty())
						name = inputs->at(0).Name;
					ZONG_CORE_ASSERT(!name.empty());
					return name;
				};

				switch (*optType)
				{
					case EPropertyType::Input: newNode = AG::AnimationGraphNodeFactory::SpawnGraphPropertyNode(graph, getVariableName(), *optType); break;
					case EPropertyType::Output: newNode = AG::AnimationGraphNodeFactory::SpawnGraphPropertyNode(graph, getVariableName(), *optType); break;
					case EPropertyType::LocalVariable: newNode = AG::AnimationGraphNodeFactory::SpawnGraphPropertyNode(graph, getVariableName(), *optType, (bool)candidate.NumOutputs); break;
					default:
						ZONG_CORE_ASSERT(false, "Deserialized invalid AnimationGraph property type.");
						break;
				}
			}
			else
			{
				if (candidate.Type == NodeType::Comment)
				{
					// Comments are special case where the name is editable and is displayed in the comment header
					//! Alternatively we could make a special pin to use as the header for the comment node, just to keep this logic more generic.
					newNode = AG::AnimationGraphNodeFactory::SpawnNodeStatic(candidate.Category, magic_enum::enum_name<NodeType::Comment>());
					newNode->Name = candidate.Name;
				}
				else
				{
					if (candidate.Category == "Transition")
					{
						newNode = AG::TransitionGraphNodeFactory::SpawnNodeStatic(candidate.Category, candidate.Name);
					}
					else if (candidate.Category == "StateMachine")
					{
						if (candidate.Name == "Transition")
						{
							// Transition nodes are not registered. Need to create them directly.
							newNode = AG::StateMachineNodeFactory::CreateTransitionNode();
							newNode->Category = candidate.Category;
						}
						else
						{
							newNode = AG::StateMachineNodeFactory::SpawnNodeStatic(candidate.Category, candidate.Name);
						}

						// State machine nodes can have dynamic number of "transition" inputs/outputs.
						// Create them all here.
						for (const PinCandidate& in : *inputs)
						{
							if (in.Name == "Transition")
							{
								auto pin = newNode->Inputs.emplace_back(AG::Types::CreatePinForType(AG::Types::EPinType::Flow, "Transition"));
								pin->ID = in.ID;
								pin->NodeID = newNode->ID;
								pin->Kind = PinKind::Input;
							}
						}

						for (const PinCandidate& out : *outputs)
						{
							if (out.Name == "Transition")
							{
								auto pin = newNode->Outputs.emplace_back(AG::Types::CreatePinForType(AG::Types::EPinType::Flow, "Transition"));
								pin->ID = out.ID;
								pin->NodeID = newNode->ID;
								pin->Kind = PinKind::Output;
							}
						}
					}
					else
					{
						newNode = AG::AnimationGraphNodeFactory::SpawnNodeStatic(candidate.Category, candidate.Name);
					}
				}
			}

			ZONG_CORE_ASSERT(!newNode || (newNode->Type == candidate.Type));
			if (newNode)
			{
				newNode->Description = candidate.Description;
			}

			return newNode;
		};

		auto deserializePin = [](const PinCandidate& candidate, Pin* factoryPin, const NodeCandidate& node)
		{
			auto optType = magic_enum::enum_cast<AG::Types::EPinType>(candidate.GetTypeString());

			if (!optType.has_value())
				return false;


			// Check the type is correct
			const AG::Types::EPinType candidateType = *optType;

			if (candidateType != factoryPin->GetType())
			{
				const std::string_view candidateTypeStr = magic_enum::enum_name<AG::Types::EPinType>(candidateType);
				const std::string_view factoryPinTypeStr = factoryPin->GetTypeString();

				ZONG_CONSOLE_LOG_ERROR(
					"Pin type of the deserialized Node Pin ({0} - '{1}' ({2})) does not match the Pin type of the factory Node Pin ({0} - '{3}' ({4})).",
					node.Name, candidate.Name, candidateTypeStr, factoryPin->Name, factoryPinTypeStr
				);
				return false;
			}
			else if (candidate.Name != factoryPin->Name)
			{
				// TODO: JP. this may not be correct when/if we change the Comment node to use Pin to display text instead of Node name
				ZONG_CONSOLE_LOG_ERROR(
					"Pin name of the deserialized Node Pin ({0} - '{1}') does not match the Pin name of the factory Node Pin ({0} - '{2}').",
					node.Name, candidate.Name, factoryPin->Name
				);
				return false;
			}

			// Flow types are not serialized
			if (factoryPin->IsType(AG::Types::Flow))
				return true;

			// TODO: JP. for now we use 'void' and 'string' for connected pins (this is needed for graph parsing,
			//           but perhaps we could assign 'void' type for the compilation step only?)
			//           note: AnimationAsset is "AnimationAsset" type in the asset, but int64_t in the factory
			if (candidate.Value.getType() == factoryPin->Value.getType() || Utils::IsAssetHandle<AssetType::Animation>(candidate.Value) || candidate.Value.isVoid() || candidate.Value.isString())
			{
				factoryPin->Value = candidate.Value;
				return true;
			}
			else
			{
				// This can trigger for local variable nodes with array values if the value was set via the model API (e.g. model->GetLocalVariables().Set("thing", value) after the graph was created.
				// (in this case the factory pin value might have a different number of elements to the candidate pin, and so fails the value type check.  It does not matter, so ignore it here)
				if (node.Category == "Local Variable" && node.NumInputs == 0 && node.NumOutputs == 1)
				{
					return true;
				}

				// This can trigger if we've trying to load old version of a Node.
				// TODO (0x): make better...  Instead of failing to load, just set some sensible default value and carry on.
				ZONG_CONSOLE_LOG_ERROR(
					"Value type of the deserialized Node Pin ({0} - '{1}') does not match the Value type of the factory Node Pin ({0} - '{2}').",
					node.Name, candidate.Name, factoryPin->Name
				);
				return false;
			}
		};

		DefaultGraphSerializer::DeserializationFactory factory{ constructNode, deserializePin };

		auto deserializeGraph = [&graph, &factory](UUID id, YAML::Node& node) {
			if (!node["Nodes"] || !node["Links"]) return false;

			try
			{
				DefaultGraphSerializer::TryLoadNodes(node, graph->m_Nodes[id], factory);
			}
			catch (const std::exception& e)
			{
				ZONG_CONSOLE_LOG_FATAL(e.what());
				return false;
			}

			// Assign enum tokens to pins
			for (auto& node : graph->m_Nodes[id])
			{
				for (auto& pin : node->Inputs)
				{
					if (pin->IsType(AG::Types::EPinType::Enum))
					{
						if (const auto* tokens = AG::AnimationGraphNodeFactory::GetEnumTokens(node->Name, pin->Name))
							pin->EnumTokens = tokens;
					}
				}
			}

			// Links
			DefaultGraphSerializer::TryLoadLinks(node, graph->m_Links[id]);

			// Graph State
			ZONG_DESERIALIZE_PROPERTY(GraphState, graph->m_GraphState[id], node, std::string());
			return true;
		};

		if (data["Nodes"])
		{
			if (!deserializeGraph(0, data))
			{
				graph = Ref<AnimationGraphAsset>::Create();
				return false;
			}
		}
		else
		{
			for (auto element : data)
			{
				auto key = element.first.as<std::string>();
				if (key != "Skeleton" && key != "GraphInputs" && key != "LocalVariables" && key != "GraphOutputs" && key != "GraphState")
				{
					if (!deserializeGraph(element.first.as<UUID>(), element.second))
					{
						graph = Ref<AnimationGraphAsset>::Create();
						return false;
					}
				}
			}
		}

		return true;
	}


	void AnimationGraphAssetSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<AnimationGraphAsset> animationGraph = asset.As<AnimationGraphAsset>();

		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		ZONG_CORE_VERIFY(fout.good());

		std::string yamlString = SerializeToYAML(animationGraph);
		fout << yamlString;
	}


	bool AnimationGraphAssetSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		Ref<AnimationGraphAsset> animationGraph;
		if (TryLoadData(Project::GetEditorAssetManager()->GetFileSystemPath(metadata), animationGraph))
		{
			asset = animationGraph;
			asset->Handle = metadata.Handle;

			// TODO: instead of compile-on-load, maybe save compiled prototype somewhere
			AnimationGraphNodeEditorModel model(animationGraph);
			if (!model.GetNodes().empty())
			{
				// Ensure that every state machine node in the graph has an entry state set.
				// This sets the first state in each state machine as the entry state.
				// When state machine nodes are serialized, we make sure that the entry state is serialized first.
				model.EnsureEntryStates();
				model.CompileGraph(); // It could fail to compile.  That's OK, we've still successfully loaded something
			}

			return true;
		}
		return false;
	}


	bool AnimationGraphAssetSerializer::TryLoadData(const std::filesystem::path& path, Ref<AnimationGraphAsset>& asset)
	{
		std::ifstream stream(path);
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<AnimationGraphAsset> animationGraph;
		if (DeserializeFromYAML(strStream.str(), animationGraph))
		{
			if (asset)
				animationGraph->Handle = asset->Handle;

			asset = animationGraph;
			return true;  // We have loaded something.  It could be invalid.
		}
		return false;
	}


	bool AnimationGraphAssetSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<AnimationGraphAsset> animationGraph = AssetManager::GetAsset<AnimationGraphAsset>(handle);
		std::string yamlString = SerializeToYAML(animationGraph);

		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}


	Ref<Asset> AnimationGraphAssetSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<AnimationGraphAsset> animationGraph;
		bool result = DeserializeFromYAML(yamlString, animationGraph);
		if (!result)
			return nullptr;

		// TODO: for runtime we should be packing/loading just the prototype, and no need to compile here
		AnimationGraphNodeEditorModel model(animationGraph);
		model.CompileGraph(); // It could fail to compile.  That's OK, we've still successfully loaded something

		return animationGraph;
	}

}
