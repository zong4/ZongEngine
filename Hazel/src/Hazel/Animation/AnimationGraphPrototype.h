#pragma once
#include "Hazel/Asset/Asset.h"
#include "Hazel/Core/Base.h"
#include "Hazel/Core/Identifier.h"
#include "Hazel/Core/UUID.h"

#include <choc/containers/choc_Value.h>


namespace Hazel::AnimationGraph {

	struct Prototype
	{
		std::string DebugName;
		UUID ID;

		AssetHandle Skeleton = 0;

		Prototype(AssetHandle skeleton) : Skeleton(skeleton) {}

		struct Endpoint
		{
			Identifier ID;
			choc::value::Value DefaultValue;

			Endpoint(Identifier id, choc::value::ValueView defaultValue) : ID(id), DefaultValue(defaultValue) {}

			Endpoint() : ID(0), DefaultValue() {}
		};
		std::vector<Endpoint> Inputs;
		std::vector<Endpoint> Outputs;
		std::vector<Endpoint> LocalVariablePlugs;

		struct Node
		{
			Identifier TypeID; // Used to call Factory to create the right node type
			UUID ID;
			std::vector<Endpoint> DefaultValuePlugs;

			Scope<Prototype> Subroutine;

			Node(Identifier typeID, UUID id) : TypeID(typeID), ID(id) {}
			Node() : TypeID(0), ID(0) {}
		};
		std::vector<Node> Nodes;

		struct Connection
		{
			enum EType
			{
				NodeValue_NodeValue = 0,
				NodeEvent_NodeEvent = 1,
				GraphValue_NodeValue = 2,
				GraphValue_GraphValue = 4, // In "transition" graphs, it is possible to connect an input variable directly to graph output. E.g a simple boolean input that activates the transition
				GraphEvent_NodeEvent = 5,
				NodeValue_GraphValue = 6,
				NodeEvent_GraphEvent = 7,
				LocalVariable_NodeValue = 8,
			};

			struct Endpoint
			{
				UUID NodeID;
				Identifier EndpointID;
			};
			Endpoint Source;
			Endpoint Destination;

			EType Type;

			Connection(Endpoint&& source, Endpoint&& destination, EType connectionType) : Source(source), Destination(destination), Type(connectionType) {}
			Connection() : Source{ 0, 0 }, Destination{ 0, 0 }, Type(NodeValue_NodeValue) {}
		};
		std::vector<Connection> Connections;

	};

} // namespace Hazel::AnimationGraph
