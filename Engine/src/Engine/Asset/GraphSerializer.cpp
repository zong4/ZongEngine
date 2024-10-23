#include "pch.h"
#include "GraphSerializer.h"

#include "Engine/Editor/NodeGraphEditor/NodeEditorModel.h"
#include "Engine/Editor/NodeGraphEditor/NodeGraphUtils.h"

#include "Engine/Utilities/SerializationMacros.h"
#include "Engine/Utilities/YAMLSerializationHelpers.h"

#include "choc/text/choc_JSON.h"
#include "magic_enum.hpp"

namespace Hazel {

	namespace Utils {

		std::string StorageKindToString(StorageKind storageKind)
		{
			return std::string(magic_enum::enum_name<StorageKind>(storageKind));
		}


		StorageKind StorageKindFromString(const std::string_view& storageKindStr)
		{
			if (auto optValue = magic_enum::enum_cast<StorageKind>(storageKindStr))
				return *optValue;

			ZONG_CORE_ASSERT(false, "Unknown Storage Kind");
			return StorageKind::Value;
		}


		std::string NodeTypeToString(NodeType nodeType)
		{
			return std::string(magic_enum::enum_name<NodeType>(nodeType));
		}


		NodeType NodeTypeFromString(const std::string_view& nodeTypeStr)
		{
			if (auto optValue = magic_enum::enum_cast<NodeType>(nodeTypeStr))
				return *optValue;

			ZONG_CORE_ASSERT(false, "Unknown Node Type");
			return NodeType::Simple;
		}
	}

	//! Old Node Graph serialization, kept as an example
#if 0
	void DefaultGraphSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<SoundGraphAsset> graph = asset.As<SoundGraphAsset>();

		// Out
		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		ZONG_CORE_VERIFY(fout.good());
		std::string yamlString = SerializeToYAML(graph);
		fout << yamlString;
	}

	bool DefaultGraphSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		std::ifstream stream(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		if (!stream.is_open())
			return false;

		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<SoundGraphAsset> soundGraphAsset;
		if (!DeserializeFromYAML(strStream.str(), soundGraphAsset))
			return false;

		asset = soundGraphAsset;
		asset->Handle = metadata.Handle;
		return true;
	}

	bool DefaultGraphSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<SoundGraphAsset> soundGraph = AssetManager::GetAsset<SoundGraphAsset>(handle);

		std::string yamlString = SerializeToYAML(soundGraph);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}

	Ref<Asset> DefaultGraphSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		ZONG_CORE_VERIFY(false, "DefaultGraphSerialized should not be used.");

		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<SoundGraphAsset> soundGraph;
		bool result = DeserializeFromYAML(yamlString, soundGraph);
		if (!result)
			return nullptr;

		return soundGraph;
	}
#endif

	void DefaultGraphSerializer::SerializeNodes(YAML::Emitter& out, const std::vector<Node*>& nodes)
	{
		out << YAML::Key << "Nodes" << YAML::Value;
		out << YAML::BeginSeq;
		for (const auto node : nodes)
		{
			out << YAML::BeginMap; // node

			const ImVec4& nodeCol = node->Color.Value;
			const ImVec2& nodeSize = node->Size;
			const glm::vec4 nodeColOut(nodeCol.x, nodeCol.y, nodeCol.z, nodeCol.w);
			const glm::vec2 nodeSizeOut(nodeSize.x, nodeSize.y);

			ZONG_SERIALIZE_PROPERTY(ID, node->ID, out);
			ZONG_SERIALIZE_PROPERTY(Category, node->Category, out);
			ZONG_SERIALIZE_PROPERTY(Name, node->Name, out);
			if (!node->Description.empty())
			{
				ZONG_SERIALIZE_PROPERTY(Description, node->Description, out);
			}
			ZONG_SERIALIZE_PROPERTY(Color, nodeColOut, out);
			ZONG_SERIALIZE_PROPERTY(Type, Utils::NodeTypeToString(node->Type), out);
			ZONG_SERIALIZE_PROPERTY(Size, nodeSizeOut, out);
			ZONG_SERIALIZE_PROPERTY(Location, node->State, out);

			out << YAML::Key << "Inputs" << YAML::BeginSeq;
			for (auto in : node->Inputs)
			{
				out << YAML::BeginMap; // in

				ZONG_SERIALIZE_PROPERTY(ID, in->ID, out);
				ZONG_SERIALIZE_PROPERTY(Name, in->Name, out);
				ZONG_SERIALIZE_PROPERTY(Type, std::string(in->GetTypeString()), out);
				ZONG_SERIALIZE_PROPERTY(Storage, Utils::StorageKindToString(in->Storage), out);
				ZONG_SERIALIZE_PROPERTY(Value, in->Value, out);

				out << YAML::EndMap; // in
			}
			out << YAML::EndSeq; // Inputs

			out << YAML::Key << "Outputs" << YAML::BeginSeq;
			for (auto outp : node->Outputs)
			{
				out << YAML::BeginMap; // outp
				ZONG_SERIALIZE_PROPERTY(ID, outp->ID, out);
				ZONG_SERIALIZE_PROPERTY(Name, outp->Name, out);

				ZONG_SERIALIZE_PROPERTY(Type, std::string(outp->GetTypeString()), out);
				ZONG_SERIALIZE_PROPERTY(Storage, Utils::StorageKindToString(outp->Storage), out);
				ZONG_SERIALIZE_PROPERTY(Value, outp->Value, out);

				out << YAML::EndMap; // outp
			}
			out << YAML::EndSeq; // Outputs

			out << YAML::EndMap; // node
		}
		out << YAML::EndSeq; // Nodes
	}


	void DefaultGraphSerializer::SerializeLinks(YAML::Emitter& out, const std::vector<Link>& links)
	{
		out << YAML::Key << "Links" << YAML::Value;
		out << YAML::BeginSeq;
		for (const auto& link : links)
		{
			out << YAML::BeginMap; // link

			const auto& col = link.Color.Value;
			const glm::vec4 colOut(col.x, col.y, col.z, col.w);

			ZONG_SERIALIZE_PROPERTY(ID, link.ID, out);
			ZONG_SERIALIZE_PROPERTY(StartPinID, link.StartPinID, out);
			ZONG_SERIALIZE_PROPERTY(EndPinID, link.EndPinID, out);
			ZONG_SERIALIZE_PROPERTY(Color, colOut, out);

			out << YAML::EndMap; // link
		}
		out << YAML::EndSeq; // Links
	}


	using PinCandidate = DefaultGraphSerializer::DeserializationFactory::PinCandidate;
	using NodeCandidate = DefaultGraphSerializer::DeserializationFactory::NodeCandidate;

	[[nodiscard]] std::optional<std::vector<PinCandidate>> TryLoadInputs(const YAML::Node& inputs, const NodeCandidate& node)
	{
		if (!inputs.IsSequence())
			return {};

		std::vector<PinCandidate> list;

		uint32_t index = 0;
		for (const auto& in : inputs)
		{
			UUID ID;
			std::string pinName;
			//std::string valueStr;
			choc::value::Value value;
			std::string pinType;
			std::string pinStorage;

			ZONG_DESERIALIZE_PROPERTY(ID, ID, in, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(Name, pinName, in, std::string());
			ZONG_DESERIALIZE_PROPERTY(Type, pinType, in, std::string());
			ZONG_DESERIALIZE_PROPERTY(Storage, pinStorage, in, std::string());
			//ZONG_DESERIALIZE_PROPERTY(Value, valueStr, in, std::string());
			ZONG_DESERIALIZE_PROPERTY(Value, value, in, choc::value::Value());

			// TODO: load legacy saved valueStr, or manually rewrite YAML files for the new format?

			PinCandidate candidate;
			candidate.ID = ID;
			candidate.Name = pinName;
			candidate.Storage = Utils::StorageKindFromString(pinStorage);
			candidate.Value = value;
			candidate.Kind = PinKind::Input;
			candidate.TypeString = pinType;

			list.push_back(candidate);
#if 0
			bool isCustomValueType = choc::text::contains(valueStr, "Value");

			auto parseCustomValueType = [](const std::string& valueString) -> choc::value::Value
			{
				choc::value::Value value = choc::json::parse(valueString);

				if (value["TypeName"].isVoid())
				{
					ZONG_CORE_ASSERT(false, "Failed to deserialize custom value type, missing \"TypeName\" property.");
					return {};
				}

				choc::value::Value customObject = choc::value::createObject(value["TypeName"].get<std::string>());
				if (value.isObject())
				{
					for (uint32_t i = 0; i < value.size(); i++)
					{
						choc::value::MemberNameAndValue nameValue = value.getObjectMemberAt(i);
						customObject.addMember(nameValue.name, nameValue.value);
					}
				}
				else
				{
					ZONG_CORE_ASSERT(false, "Failed to load custom value type. It must be serialized as object.")
				}

				return customObject;
			};

			const EPinType type = PinTypeFromString(pinType);
			const StorageKind storage = StorageKindFromString(pinStorage);

			//? this can throw if trying tp parse super small (or weird?) float values like 9.999999974752428e-7
			choc::value::Value v = isCustomValueType ? parseCustomValueType(valueStr) : choc::json::parseValue(valueStr);

			// Chock loads int and float values from JSON string as int64 and double,
			// need to converst them back to what wee need
			if (!isCustomValueType)
			{
				if (storage != StorageKind::Array && !v.isString() && !v.isVoid()) // string can be name of connected Graph Input
				{
					switch (type)
					{
						case Hazel::EPinType::Int: v = choc::value::createInt32(v.get<int32_t>()); break;
						case Hazel::EPinType::Float: v = choc::value::createFloat32(v.get<float>()); break;
						default:
							break;
					}
				}
				else if (storage == StorageKind::Array && v.isArray() && v.size() && !v[0].isString())
				{
					for (auto& element : v)
					{
						switch (type)
						{
							case Hazel::EPinType::Int: element = choc::value::createInt32(element.get<int32_t>()); break;
							case Hazel::EPinType::Float: element = choc::value::createFloat32(element.get<float>()); break;
							default:
								break;
						}
					}
				}
			}
			newNode->Inputs.emplace_back(ID, pinName.c_str(),
										type,
										storage,
										v)
				->Kind = PinKind::Input;
#endif
		}

		return list;
	}


	[[nodiscard]] std::optional<std::vector<PinCandidate>> TryLoadOutputs(const YAML::Node& outputs, const NodeCandidate& node)
	{
		if (!outputs.IsSequence())
			return {};

		std::vector<PinCandidate> list;

		uint32_t index = 0;
		for (const auto& out : outputs)
		{
			UUID ID;
			std::string pinName;
			//std::string valueStr;
			choc::value::Value value;
			std::string pinType;
			std::string pinStorage;

			ZONG_DESERIALIZE_PROPERTY(ID, ID, out, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(Name, pinName, out, std::string());
			ZONG_DESERIALIZE_PROPERTY(Type, pinType, out, std::string());
			ZONG_DESERIALIZE_PROPERTY(Storage, pinStorage, out, std::string());
			//ZONG_DESERIALIZE_PROPERTY(Value, valueStr, out, std::string());
			ZONG_DESERIALIZE_PROPERTY(Value, value, out, choc::value::Value());

			PinCandidate candidate;
			candidate.ID = ID;
			candidate.Name = pinName;
			candidate.Storage = Utils::StorageKindFromString(pinStorage);
			candidate.Value = value;
			candidate.Kind = PinKind::Output;
			candidate.TypeString = pinType;

			list.push_back(candidate);

#if 0
			newNode->Outputs.emplace_back(ID, pinName.c_str(),
										 PinTypeFromString(pinType),
										 StorageKindFromString(pinStorage),
										 choc::json::parseValue(valueStr)) //? what if the output is of the Object type?
				->Kind = PinKind::Output;
#endif
		}

		return list;
	}


	void DefaultGraphSerializer::TryLoadNodes(YAML::Node& data, std::vector<Node*>& nodes, const DeserializationFactory& factory)
	{
		// TODO: JP. remove exceptions when we implement a dummy "invalid" node/pin types to display to the user?
		//		for now we just stop loading the whole graph if there's any error in the YAML document and/or node fabrication
		for (auto node : data["Nodes"])
		{
			UUID nodeID;
			std::string nodeCategory;
			std::string nodeName;
			std::string nodeDesc;
			std::string location;
			std::string nodeTypeStr;
			glm::vec4 nodeColor;
			glm::vec2 nodeSize;

			ZONG_DESERIALIZE_PROPERTY(ID, nodeID, node, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(Category, nodeCategory, node, std::string());
			ZONG_DESERIALIZE_PROPERTY(Name, nodeName, node, std::string());
			ZONG_DESERIALIZE_PROPERTY(Description, nodeDesc, node, std::string());
			if (node["Colour"])
			{
				ZONG_DESERIALIZE_PROPERTY(Colour, nodeColor, node, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			else
			{
				ZONG_DESERIALIZE_PROPERTY(Color, nodeColor, node, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			ZONG_DESERIALIZE_PROPERTY(Type, nodeTypeStr, node, std::string());	// TODO: YAML serialization for enums to/from string
			ZONG_DESERIALIZE_PROPERTY(Size, nodeSize, node, glm::vec2());
			ZONG_DESERIALIZE_PROPERTY(Location, location, node, std::string());

			const NodeType nodeType = Utils::NodeTypeFromString(nodeTypeStr);	// TODO: JP. this could also be implmentation specific types

			NodeCandidate candidate;
			candidate.ID = nodeID;
			candidate.Category = nodeCategory;
			candidate.Name = nodeName;
			candidate.Description = nodeDesc;
			candidate.Type = nodeType;
			candidate.NumInputs = node["Inputs"] ? (uint32_t)node["Inputs"].size() : 0;
			candidate.NumOutputs = node["Outputs"] ? (uint32_t)node["Outputs"].size() : 0;

			std::optional<std::vector<PinCandidate>> candidateInputs;
			std::optional<std::vector<PinCandidate>> candidateOutputs;

			auto inputsNode = node["Inputs"];
			if (inputsNode)
			{
				if (!(candidateInputs = TryLoadInputs(inputsNode, candidate)))
				{
					// YAML file contains "Inputs" but we've failed to parse them
					throw std::runtime_error("Failed to load editor Node inputs '" + candidate.Name + "' inputs.");
				}
				else if (candidateInputs->size() != candidate.NumInputs)
				{
					// YAML file contains different number of entries for the Inputs than we've managed to deserialize
					throw std::runtime_error("Deserialized Node Inputs list doesn't match the number of serialized Node '" + candidate.Name + "' inputs.");
				}
			}

			auto outputsNode = node["Outputs"];
			if (outputsNode)
			{
				if (!(candidateOutputs = TryLoadOutputs(outputsNode, candidate)))
				{
					// YAML file contains "Outputs" but we've failed to parse them
					throw std::runtime_error("Failed to load editor Node '" + candidate.Name + "' outputs.");
				}
				else if (candidateOutputs->size() != candidate.NumOutputs)
				{
					// YAML file contains different number of entries for the Outputs than we've managed to deserialize
					throw std::runtime_error("Deserialized Node Outputs list doesn't match the number of serialized Node '" + candidate.Name + "' outputs.");
				}
			}

			// This is not going to load old Node configurations and enforce old to new Topology compatibility
			// TODO: JP. we might want to still load old topology as an "invalid" dummy node to display it to the user
			Node* newNode = factory.ConstructNode(candidate, candidateInputs, candidateOutputs);

			if (!newNode)
			{
				throw std::runtime_error("Failed to construct deserialized Node '" + candidate.Name + "'.");
			}

			newNode->ID = candidate.ID;
			newNode->State = location;
			newNode->Color = ImColor(nodeColor.x, nodeColor.y, nodeColor.z, nodeColor.w);
			newNode->Size = ImVec2(nodeSize.x, nodeSize.y);

			if (newNode->Inputs.size() != candidate.NumInputs || newNode->Outputs.size() != candidate.NumOutputs)
			{
				//delete newNode;

				// Factory Node might have changed and we've deserialized an old version of the Node
				//throw std::runtime_error("Deserialized Node topology doesn't match factory Node '" + candidate.Name + "'.");
				ZONG_CONSOLE_LOG_WARN("Deserialized Node topology doesn't match factory Node '" + candidate.Name + "'.");
			}

			// Implementation specific construction and/or validation of Node Pins

			if (candidateInputs)
			{
				for(uint32_t i = 0; i < newNode->Inputs.size(); ++i) {
					Pin* factoryPin = newNode->Inputs[i];

					// find candidate with same name as factory pin
					PinCandidate* candidatePin = nullptr;
					for (auto& pin: *candidateInputs)
					{
						if (pin.Name == factoryPin->Name)
						{
							candidatePin = &pin;
							break;
						}
					}

					if (candidatePin)
					{
						if (!factory.DeserializePin(*candidatePin, factoryPin, candidate))
						{
							delete newNode;
							// This error is pushed by the implementation
							throw std::runtime_error("Failed to deserialize/validate input Pin '" + candidatePin->Name + "' for a Node '" + candidate.Name + "'.");
						}

						factoryPin->ID = candidatePin->ID;
						// There could be multiple candidates with same name, so remove deserialized candidate from list
						candidateInputs->erase(std::remove_if(candidateInputs->begin(), candidateInputs->end(), [candidatePin](const PinCandidate& pin) { return pin.ID == candidatePin->ID; }), candidateInputs->end());
					}
					else
					{
						factoryPin->ID = UUID();
					}
					factoryPin->NodeID = candidate.ID;
					factoryPin->Kind = PinKind::Input;
				}
			}

			if (candidateOutputs)
			{
				for (uint32_t i = 0; i < newNode->Outputs.size(); ++i)
				{
					Pin* factoryPin = newNode->Outputs[i];
					
					// find candidate with same name as factory pin
					PinCandidate* candidatePin = nullptr;
					for (auto& pin : *candidateOutputs)
					{
						if (pin.Name == factoryPin->Name)
						{
							candidatePin = &pin;
							break;
						}
					}

					if (candidatePin)
					{
						if (!factory.DeserializePin(*candidatePin, factoryPin, candidate))
						{
							delete newNode;
							// This error is pushed by the implementation
							throw std::runtime_error("Failed to deserialize/validate output Pin '" + candidatePin->Name + "' for a Node '" + candidate.Name + "'.");
						}

						factoryPin->ID = candidatePin->ID;
						// There could be multiple candidates with same name, so remove deserialized candidate from list
						candidateOutputs->erase(std::remove_if(candidateOutputs->begin(), candidateOutputs->end(), [candidatePin](const PinCandidate& pin) { return pin.ID == candidatePin->ID; }), candidateOutputs->end());
					}
					else
					{
						factoryPin->ID = UUID();
					}
					factoryPin->NodeID = candidate.ID;
					factoryPin->Kind = PinKind::Output;
				}
			}

			nodes.push_back(newNode);
		}
	}

	void DefaultGraphSerializer::TryLoadLinks(YAML::Node& data, std::vector<Link>& links)
	{
		for (auto link : data["Links"])
		{
			UUID ID;
			UUID StartPinID;
			UUID EndPinID;
			glm::vec4 color;

			ZONG_DESERIALIZE_PROPERTY(ID, ID, link, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(StartPinID, StartPinID, link, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(EndPinID, EndPinID, link, uint64_t(0));
			if (link["Colour"])
			{
				ZONG_DESERIALIZE_PROPERTY(Colour, color, link, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			else
			{
				ZONG_DESERIALIZE_PROPERTY(Color, color, link, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			links.emplace_back(StartPinID, EndPinID);
			links.back().ID = ID;
			links.back().Color = ImColor(color.x, color.y, color.z, color.w);
		}
	}

	//! Old Node Graph serialization, kept as an example
#if 0
	std::string DefaultGraphSerializer::SerializeToYAML(Ref<SoundGraphAsset> soundGraphAsset) const
	{
		YAML::Emitter out;

		out << YAML::BeginMap; // Nodes, Links, Graph State

		//============================================================
		/// Nodes

		SerializeNodes(out, soundGraphAsset->Nodes);

#if 0
		out << YAML::Key << "Nodes" << YAML::Value;
		out << YAML::BeginSeq;
		for (auto& node : graph->Nodes)
		{
			out << YAML::BeginMap; // node

			const ImVec4& nodeCol = node.Color.Value;
			const ImVec2& nodeSize = node.Size;
			const glm::vec4 nodeColOut(nodeCol.x, nodeCol.y, nodeCol.z, nodeCol.w);
			const glm::vec2 nodeSizeOut(nodeSize.x, nodeSize.y);

			ZONG_SERIALIZE_PROPERTY(ID, node.ID, out);
			ZONG_SERIALIZE_PROPERTY(Category, node.Category, out);
			ZONG_SERIALIZE_PROPERTY(Name, node.Name, out);
			ZONG_SERIALIZE_PROPERTY(Colour, nodeColOut, out);
			ZONG_SERIALIZE_PROPERTY(Type, NodeTypeToString(node.Type), out);
			ZONG_SERIALIZE_PROPERTY(Size, nodeSizeOut, out);
			ZONG_SERIALIZE_PROPERTY(Location, node.State, out);

			out << YAML::Key << "Inputs" << YAML::BeginSeq;
			for (auto& in : node.Inputs)
			{
				out << YAML::BeginMap; // in
				ZONG_SERIALIZE_PROPERTY(ID, in.ID, out);
				ZONG_SERIALIZE_PROPERTY(Name, in.Name, out);
				ZONG_SERIALIZE_PROPERTY(Type, PinTypeToString(in.Type), out);
				ZONG_SERIALIZE_PROPERTY(Storage, StorageKindToString(in.Storage), out);
				ZONG_SERIALIZE_PROPERTY(Value, choc::json::toString(in.Value), out);
				out << YAML::EndMap; // in
			}
			out << YAML::EndSeq; // Inputs

			out << YAML::Key << "Outputs" << YAML::BeginSeq;
			for (auto& outp : node.Outputs)
			{
				out << YAML::BeginMap; // outp
				ZONG_SERIALIZE_PROPERTY(ID, outp.ID, out);
				ZONG_SERIALIZE_PROPERTY(Name, outp.Name, out);
				ZONG_SERIALIZE_PROPERTY(Type, PinTypeToString(outp.Type), out);
				ZONG_SERIALIZE_PROPERTY(Storage, StorageKindToString(outp.Storage), out);
				ZONG_SERIALIZE_PROPERTY(Value, choc::json::toString(outp.Value), out);
				out << YAML::EndMap; // outp
			}
			out << YAML::EndSeq; // Outputs

			out << YAML::EndMap; // node
		}
		out << YAML::EndSeq; // Nodes
#endif

		//============================================================
		/// Links

		SerializeLinks(out, soundGraphAsset->Links);

#if 0
		out << YAML::Key << "Links" << YAML::Value;
		out << YAML::BeginSeq;
		for (auto& link : graph->Links)
		{
			out << YAML::BeginMap; // link

			const auto& col = link.Color.Value;
			const glm::vec4 colOut(col.x, col.y, col.z, col.w);

			ZONG_SERIALIZE_PROPERTY(ID, link.ID, out);
			ZONG_SERIALIZE_PROPERTY(StartPinID, link.StartPinID, out);
			ZONG_SERIALIZE_PROPERTY(EndPinID, link.EndPinID, out);
			ZONG_SERIALIZE_PROPERTY(Colour, colOut, out);

			out << YAML::EndMap; // link
		}
		out << YAML::EndSeq; // Links
#endif

		//============================================================
		/// Graph State

		ZONG_SERIALIZE_PROPERTY(GraphState, soundGraphAsset->GraphState, out);

		out << YAML::EndMap; // Nodes, Links, Graph State
		return std::string(out.c_str());
		return {};
	}

	bool DefaultGraphSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<SoundGraphAsset>& soundGraphAsset) const
	{
		YAML::Node data = YAML::Load(yamlString);

		soundGraphAsset = Ref<SoundGraphAsset>::Create();

		//============================================================
		/// Nodes

		TryLoadNodes(data, soundGraphAsset->Nodes);
#if 0
		for (auto& node : data["Nodes"])
		{
			UUID nodeID;
			std::string nodeCategory;
			std::string nodeName;
			std::string location;
			std::string nodeType;
			glm::vec4 nodeCol;
			glm::vec2 nodeS;

			ZONG_DESERIALIZE_PROPERTY(ID, nodeID, node, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(Category, nodeCategory, node, std::string());
			ZONG_DESERIALIZE_PROPERTY(Name, nodeName, node, std::string());
			ZONG_DESERIALIZE_PROPERTY(Colour, nodeCol, node, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			ZONG_DESERIALIZE_PROPERTY(Type, nodeType, node, std::string());
			ZONG_DESERIALIZE_PROPERTY(Size, nodeS, node, glm::vec2());
			ZONG_DESERIALIZE_PROPERTY(Location, location, node, std::string());

			auto& newNode = graph->Nodes.emplace_back(nodeID, nodeName.c_str());
			newNode.Category = nodeCategory;
			newNode.State = location;
			newNode.Color = ImColor(nodeCol.x, nodeCol.y, nodeCol.z, nodeCol.w);
			newNode.Type = NodeTypeFromString(nodeType);
			newNode.Size = ImVec2(nodeS.x, nodeS.y);

			if (node["Inputs"])
			{
				for (auto& in : node["Inputs"])
				{
					UUID ID;
					std::string pinName;
					std::string valueStr;
					std::string pinType;
					std::string pinStorage;

					ZONG_DESERIALIZE_PROPERTY(ID, ID, in, uint64_t(0));
					ZONG_DESERIALIZE_PROPERTY(Name, pinName, in, std::string());
					ZONG_DESERIALIZE_PROPERTY(Type, pinType, in, std::string());
					ZONG_DESERIALIZE_PROPERTY(Storage, pinStorage, in, std::string());
					ZONG_DESERIALIZE_PROPERTY(Value, valueStr, in, std::string());

					bool isCustomValueType = choc::text::contains(valueStr, "Value");

					auto parseCustomValueType = [](const std::string& valueString) -> choc::value::Value
					{
						choc::value::Value value = choc::json::parse(valueString);

						if (value["TypeName"].isVoid())
						{
							ZONG_CORE_ASSERT(false, "Failed to deserialize custom value type, missing \"TypeName\" property.");
							return {};
						}

						choc::value::Value customObject = choc::value::createObject(value["TypeName"].get<std::string>());
						if (value.isObject())
						{
							for (uint32_t i = 0; i < value.size(); i++)
							{
								choc::value::MemberNameAndValue nameValue = value.getObjectMemberAt(i);
								customObject.addMember(nameValue.name, nameValue.value);
							}
						}
						else
						{
							ZONG_CORE_ASSERT(false, "Failed to load custom value type. It must be serialized as object.")
						}

						return customObject;
					};

					newNode.Inputs.emplace_back(ID, pinName.c_str(),
												PinTypeFromString(pinType),
												StorageKindFromString(pinStorage),
												isCustomValueType ? parseCustomValueType(valueStr) : choc::json::parseValue(valueStr))
						.Kind = PinKind::Input;
				}
			}

			if (node["Outputs"])
			{
				for (auto& out : node["Outputs"])
				{
					UUID ID;
					std::string pinName;
					std::string valueStr;
					std::string pinType;
					std::string pinStorage;

					ZONG_DESERIALIZE_PROPERTY(ID, ID, out, uint64_t(0));
					ZONG_DESERIALIZE_PROPERTY(Name, pinName, out, std::string());
					ZONG_DESERIALIZE_PROPERTY(Type, pinType, out, std::string());
					ZONG_DESERIALIZE_PROPERTY(Storage, pinStorage, out, std::string());
					ZONG_DESERIALIZE_PROPERTY(Value, valueStr, out, std::string());

					newNode.Outputs.emplace_back(ID, pinName.c_str(),
												PinTypeFromString(pinType),
												StorageKindFromString(pinStorage),
												choc::json::parseValue(valueStr))
						.Kind = PinKind::Output;
				}
			}
		}
#endif

		//============================================================
		/// Links

		TryLoadLinks(data, soundGraphAsset->Links);
#if 0
		for (auto& link : data["Links"])
		{
			UUID ID;
			UUID StartPinID;
			UUID EndPinID;
			glm::vec4 colour;

			ZONG_DESERIALIZE_PROPERTY(ID, ID, link, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(StartPinID, StartPinID, link, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(EndPinID, EndPinID, link, uint64_t(0));
			ZONG_DESERIALIZE_PROPERTY(Colour, colour, link, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

			graph->Links.emplace_back(ID, StartPinID, EndPinID)
				.Color = ImColor(colour.x, colour.y, colour.z, colour.w);
		}
#endif

		//============================================================
		/// Graph State

		ZONG_DESERIALIZE_PROPERTY(GraphState, soundGraphAsset->GraphState, data, std::string());
		return true;
	}
#endif

} // namespace Hazel
