#include <pch.h>
#include "Nodes.h"

#include <imgui_node_editor_internal.h>

ImVec2 Hazel::Node::GetPosition() const
{
	ax::NodeEditor::Detail::NodeSettings settings(0);
	ax::NodeEditor::Detail::NodeSettings::Parse(State, settings);
	return settings.m_Location;
}


void Hazel::Node::SetPosition(ImVec2 pos)
{
	ax::NodeEditor::Detail::NodeSettings settings(0);
	ax::NodeEditor::Detail::NodeSettings::Parse(State, settings);
	settings.m_Location = pos;
	State = settings.Serialize().dump();
}

// These are just examples and are not actually needed to build Sound or Animation node graphs
#if 0
#include "choc/text/choc_StringUtilities.h"

namespace Hazel::Nodes {

	struct ExamplePin : Pin
	{
		using Pin::Pin;

		ExamplePin(UUID ID, std::string_view name, EPinType type)
			: Pin(ID, name), Type(type)
		{
		}

		int GetType() const override { return (int)Type; }
		std::string_view GetTypeString() const override { return magic_enum::enum_name<EPinType>(Type); }

		EPinType Type;
	};

	struct ExampleNode : Node
	{
		using Node::Node;

		virtual int GetTypeID() const override { return 0; }
	};

	//==========================================================================
	/// Utility Nodes
	class Utility
	{
	public:
		static Node* Comment()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new ExampleNode(0, nodeName.c_str());
			node->Category = "Utility";

			node->Type = NodeType::Comment;
			node->Size = ImVec2(300, 200);

			return node;
		}

		static Node* Message()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new ExampleNode(0, nodeName.c_str(), ImColor(128, 195, 248));
			node->Category = "Utility";

			node->Type = NodeType::Simple;
			node->Outputs.push_back(new ExamplePin(0, "Message", EPinType::String));

			return node;
		}

		static Node* Dummy_Node()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new ExampleNode(0, nodeName.c_str(), ImColor(128, 195, 248));
			node->Category = "Utility";

			node->Type = NodeType::Blueprint;

			node->Inputs.push_back(new ExamplePin(0, "Message", EPinType::String));

			node->Outputs.push_back(new ExamplePin(0, "Message", EPinType::String));

			return node;
		}
	};

} // namespace Hazel::Nodes
#endif

