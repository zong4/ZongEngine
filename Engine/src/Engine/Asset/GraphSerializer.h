#pragma once

#include "AssetSerializer.h"

#include "Engine/Editor/NodeGraphEditor/Nodes.h"

#include <yaml-cpp/yaml.h>

#include <string>

namespace Engine {

	namespace Utils {
		std::string PinTypeToString(EPinType pinType);
		EPinType PinTypeFromString(const std::string_view& pinTypeStr);

		std::string StorageKindToString(StorageKind storageKind);
		StorageKind StorageKindFromString(const std::string_view& storageKindStr);

		std::string NodeTypeToString(NodeType nodeType);
		NodeType NodeTypeFromString(const std::string_view& nodeTypeStr);

	} // namespace Utils


	//==================================================================================
	/*	Graph serializer base provides static utilities for implementation
		to serialize and deserialize graph data.

		Implementation can deserialize nodes on its own, or use handy static
		TryLoadNodes() utility, which first deserializes the information about potential
		Node and Pins and then passes this info to the implementation factory callbacks
		to handle concrete construction and validation.
	*/
	class DefaultGraphSerializer : public AssetSerializer
	{
	public:

		static void SerializeNodes(YAML::Emitter& out, const std::vector<Node*>& nodes);
		static void SerializeLinks(YAML::Emitter& out, const std::vector<Link>& links);

		//==================================================================================
		/*	Implementation must provide this factory to deserialize nodes with static
			TryLoadNodes() function.
		*/
		struct DeserializationFactory
		{
			// Deserialized info about a Pin of a Node, may or may not be valid
			struct PinCandidate : Pin
			{
				using Pin::Pin;

				std::string TypeString;	// implementation specific

				int GetType() const override { return -1; }
				std::string_view GetTypeString() const override { return TypeString; }
			};

			// Deserialized info about a Node, may or may not be valid
			struct NodeCandidate
			{
				UUID ID;
				std::string Category;
				std::string Name;
				std::string Description;
				NodeType Type;
				uint32_t NumInputs;
				uint32_t NumOutputs;
			};

			/*	This factory function should construct a Node with default Input and Output pins
				as well as assign default and deserialized values from the candidate if required.
			*/
			std::function<Node*(const NodeCandidate& candidate,
								const std::optional<std::vector<PinCandidate>>& inputs,
								const std::optional<std::vector<PinCandidate>>& outputs)> ConstructNode;
			
			/*	This factory function should deserialize values from the candidate to the previously
				constructed default factory pin as well as do any required validation.
			*/
			std::function<bool(const PinCandidate& candidate,
								Pin* factoryPin,
								const NodeCandidate& nodeCandidate)> DeserializePin;
		};

		/*	Try to load graph Nodes from YAML. This function parses YAML into NodeCandidates and PinCandidates,
			which are then passed to implementation provided factory to deserialize and validate.

			This function throws an exception if deserialization fails, the caller must handle it!
		*/
		static void TryLoadNodes(YAML::Node& in, std::vector<Node*>& nodes, const DeserializationFactory& factory);
		static void TryLoadLinks(YAML::Node& in, std::vector<Link>& links);
	};

} // namespace Engine
