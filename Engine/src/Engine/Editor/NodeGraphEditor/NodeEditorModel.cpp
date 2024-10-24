#include <pch.h>
#include "NodeEditorModel.h"
#include "NodeGraphEditor.h"
#include "NodeGraphUtils.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Utilities/ContainerUtils.h"

#include "choc/text/choc_StringUtilities.h"
#include "imgui-node-editor/imgui_node_editor.h"

#include <sstream>

namespace Engine {
	//=============================================================================
	/// IDs

#pragma region IDs

	uintptr_t NodeEditorModel::GetIDFromString(const std::string& idString) const
	{
		std::stringstream stream(idString);
		uintptr_t ret;
		stream >> ret;
		return ret;
	}

	void NodeEditorModel::AssignPinIds(Node* node)
	{
		for (auto& input : node->Inputs)
		{
			input->ID = UUID();
			input->Kind = PinKind::Input;
			input->NodeID = node->ID;
		}


		for (auto& output : node->Outputs)
		{
			output->ID = UUID();
			output->Kind = PinKind::Output;
			output->NodeID = node->ID;
		}
	}

	void NodeEditorModel::OnNodeSpawned(Node* node)
	{
		if (node)
		{
			AssignPinIds(node);
			GetNodes().push_back(node);
		}
	}

#pragma endregion IDs


	//=============================================================================
	/// Find items

#pragma region Find objects

	Node* NodeEditorModel::FindNode(UUID id)
	{
		for (auto node : GetNodes())
			if (node->ID == id)
				return node;

		return nullptr;
	}

	const Node* NodeEditorModel::FindNode(UUID id) const
	{
		for (auto node : GetNodes())
			if (node->ID == id)
				return node;

		return nullptr;
	}

	Link* NodeEditorModel::FindLink(UUID id)
	{
		for (auto& link : GetLinks())
			if (link.ID == id)
				return &link;

		return nullptr;
	}

	const Link* NodeEditorModel::FindLink(UUID id) const
	{
		for (auto& link : GetLinks())
			if (link.ID == id)
				return &link;

		return nullptr;
	}

	Pin* NodeEditorModel::FindPin(UUID id)
	{
		if (!id)
			return nullptr;

		for (auto& node : GetNodes())
		{
			for (auto& pin : node->Inputs)
				if (pin->ID == id)
					return pin;

			for (auto& pin : node->Outputs)
				if (pin->ID == id)
					return pin;
		}

		return nullptr;
	}

	const Pin* NodeEditorModel::FindPin(UUID id) const
	{
		if (!id)
			return nullptr;

		for (auto& node : GetNodes())
		{
			for (auto& pin : node->Inputs)
				if (pin->ID == id)
					return pin;

			for (auto& pin : node->Outputs)
				if (pin->ID == id)
					return pin;
		}

		return nullptr;
	}

#pragma endregion Find items


	//=============================================================================
	/// Links

#pragma region Links

	Link* NodeEditorModel::GetLinkConnectedToPin(UUID id)
	{
		if (!id)
			return nullptr;

		for (auto& link : GetLinks())
			if (link.StartPinID == id || link.EndPinID == id)
				return &link;

		return nullptr;
	}

	const Link* NodeEditorModel::GetLinkConnectedToPin(UUID id) const
	{
		if (!id)
			return nullptr;

		for (auto& link : GetLinks())
			if (link.StartPinID == id || link.EndPinID == id)
				return &link;

		return nullptr;
	}

	const Node* NodeEditorModel::GetNodeForPin(UUID pinID) const
	{
		if (!pinID)
			return nullptr;

		for (const auto& node : GetNodes())
		{
			for (auto& pin : node->Inputs)
				if (pin->ID == pinID)
					return node;

			for (auto& pin : node->Outputs)
				if (pin->ID == pinID)
					return node;
		}

		return nullptr;
	}

	const Node* NodeEditorModel::GetNodeConnectedToPin(UUID pinID) const
	{
		const Pin* pin = FindPin(pinID);
		if (!pin) return nullptr;
		const Link* link = GetLinkConnectedToPin(pin->ID);
		if (!link) return nullptr;
		const Pin* otherPin = FindPin(pin->Kind == PinKind::Input ? link->StartPinID : link->EndPinID);
		if (!otherPin) return nullptr;

		return FindNode(otherPin->NodeID);
	}

	Node* NodeEditorModel::GetNodeConnectedToPin(UUID pinID)
	{
		Pin* pin = FindPin(pinID);
		if (!pin) return nullptr;
		Link* link = GetLinkConnectedToPin(pin->ID);
		if (!link) return nullptr;
		Pin* otherPin = FindPin(pin->Kind == PinKind::Input ? link->StartPinID : link->EndPinID);
		if (!otherPin) return nullptr;

		return FindNode(otherPin->NodeID);
	}

	bool NodeEditorModel::IsPinLinked(UUID pinID)
	{
		return GetLinkConnectedToPin(pinID) != nullptr;
	}

	bool NodeEditorModel::IsPinLinked(UUID pinID) const
	{
		return GetLinkConnectedToPin(pinID) != nullptr;
	}

	std::vector<Link*> NodeEditorModel::GetAllLinksConnectedToPin(UUID pinID)
	{
		if (!pinID)
			return {};

		std::vector<Link*> links;

		for (auto& link : GetLinks())
			if (link.StartPinID == pinID || link.EndPinID == pinID)
				links.push_back(&link);

		return links;
	}

	std::vector<UUID> NodeEditorModel::GetAllLinkIDsConnectedToPin(UUID pinID)
	{
		if (!pinID)
			return {};

		std::vector<UUID> links;

		for (auto& link : GetLinks())
			if (link.StartPinID == pinID || link.EndPinID == pinID)
				links.push_back(link.ID);

		return links;
	}

	void NodeEditorModel::DeleteDeadLinks(UUID nodeId)
	{
		auto wasConnectedToTheNode = [&](const Link& link)
		{
			return (!FindPin(link.StartPinID)) || (!FindPin(link.EndPinID))
				|| FindPin(link.StartPinID)->NodeID == nodeId
				|| FindPin(link.EndPinID)->NodeID == nodeId;
		};

		auto& links = GetLinks();

		// Clear pin value
		std::for_each(links.begin(), links.end(), [&](Link& link)
		{
			auto pin = FindPin(link.StartPinID);
			ZONG_CORE_ASSERT(pin); // this can fail if node implementation has been changed since the graph was first created
			if (pin && pin->NodeID == nodeId)
			{
				if (auto* pin = FindPin(link.EndPinID))
					AssignSomeDefaultValue(pin);
			}
		});

		auto removeIt = std::remove_if(links.begin(), links.end(), wasConnectedToTheNode);
		const bool linkRemoved = removeIt != links.end();

		links.erase(removeIt, links.end());

		if (linkRemoved)
			OnLinkDeleted();
	}

	void NodeEditorModel::AssignSomeDefaultValue(Pin* pin)
	{
		if (pin->Kind == PinKind::Output) // implementation specific
			return;

		choc::value::Value v;
		switch (pin->GetType())
		{
			case EPinType::Bool:    v = choc::value::Value(true); break;
			case EPinType::Float:   v = choc::value::Value(1.0f); break;
			case EPinType::Int:     v = choc::value::Value(int(0)); break;
			case EPinType::Enum:    v = choc::value::Value(int(0)); break;
			case EPinType::String:  v = choc::value::Value(""); break;
			default: break;
		}

		if (pin->Storage == StorageKind::Value)
			pin->Value = v;
		else
			pin->Value = choc::value::createArray(1, [&v](uint32_t) { return v; });
	}

	NodeEditorModel::LinkQueryResult NodeEditorModel::CanCreateLink(Pin* startPin, Pin* endPin)
	{
		if (!startPin)                                  return LinkQueryResult::InvalidStartPin;
		else if (!endPin)                               return LinkQueryResult::InvalidEndPin;
		else if (endPin == startPin)                    return LinkQueryResult::SamePin;
		else if (endPin->NodeID == startPin->NodeID)    return LinkQueryResult::SameNode;
		else if (endPin->Kind == startPin->Kind)        return LinkQueryResult::IncompatiblePinKind;
		else if (endPin->Storage != startPin->Storage)  return LinkQueryResult::IncompatibleStorageKind;
		else if (!endPin->IsSameType(startPin))			return LinkQueryResult::IncompatibleType;

		return LinkQueryResult::CanConnect;
	}

	void NodeEditorModel::CreateLink(Pin* startPin, Pin* endPin)
	{
		ZONG_CORE_ASSERT(startPin && endPin);

		auto& links = GetLinks();

		links.emplace_back(startPin->ID, endPin->ID);
		
		// TODO: get static Types factory from CRTP type and get icon colour from it?
		links.back().Color = GetNodeFactory()->GetIconColor(startPin->GetType());

		// Clear pin value
		endPin->Value = choc::value::Value(); //? this is not ideal, as it clears the type as well

		OnLinkCreated(links.back().ID);
	}

	void NodeEditorModel::RemoveLink(UUID linkId)
	{
		auto& links = GetLinks();

		auto id = std::find_if(links.begin(), links.end(), [linkId](auto& link) { return link.ID == linkId; });
		if (id != links.end())
		{
			// Clear pin value
			if (auto* endPin = FindPin(id->EndPinID))
				AssignSomeDefaultValue(endPin);

			links.erase(id);

			OnLinkDeleted();
		}
	}

	void NodeEditorModel::RemoveLinks(const std::vector<UUID>& linkIds)
	{
		for (const auto& linkId : linkIds)
			RemoveLink(linkId);
	}

#pragma endregion Links


	//=============================================================================
	/// Nodes

#pragma region Nodes

	void NodeEditorModel::RemoveNode(UUID nodeId)
	{
		auto& nodes = GetNodes();

		auto it = std::find_if(nodes.begin(), nodes.end(), [nodeId](auto& node) { return node->ID == nodeId; });

		if (it != nodes.end())
		{
			DeleteDeadLinks(nodeId);
			delete *it;
			nodes.erase(it);
			OnNodeDeleted();
		}
	}

	void NodeEditorModel::RemoveNodes(const std::vector<UUID>& nodeIds)
	{
		auto& nodes = GetNodes();

		for (const auto& id : nodeIds)
			DeleteDeadLinks(id);

		auto it = std::stable_partition(nodes.begin(), nodes.end(), [&nodeIds](auto& node)
		{
			for (const auto& id : nodeIds)
			{
				if (node->ID == id)
					return false;
			}
			return true;
		});

		if (it != nodes.end())
		{
			std::for_each(it, nodes.end(), [](auto node) { delete node; });
			nodes.erase(it, nodes.end());
			OnNodeDeleted();
		}
	}

	Node* NodeEditorModel::CreateNode(const std::string& category, const std::string& typeID)
	{
		if (auto* newNode = GetNodeFactory()->SpawnNode(category, typeID))
		{
			OnNodeSpawned(newNode);
			OnNodeCreated();
			return GetNodes().back();
		}
		else
		{
			return nullptr;
		}
	}

#pragma endregion Nodes


	//=============================================================================
	/// Serialization

#pragma region Serialization

	void NodeEditorModel::SaveAll()
	{
		Serialize();
	}

	void NodeEditorModel::LoadAll()
	{
		Deserialize();
	}

#pragma endregion Serialization

	//=============================================================================
	/// Model Interface

	void NodeEditorModel::CompileGraph()
	{
		OnCompileGraph();
	}


	//=============================================================================
	// IO Node Editor Model
	//=============================================================================

	std::string IONodeEditorModel::GetPropertyToken(EPropertyType type)
	{
		return Utils::SplitAtUpperCase(magic_enum::enum_name<EPropertyType>(type));
	}


	EPropertyType IONodeEditorModel::GetPropertyType(std::string_view propertyToken)
	{
		if (propertyToken == GetPropertyToken(EPropertyType::Input))
			return EPropertyType::Input;
		else if (propertyToken == GetPropertyToken(EPropertyType::Output))
			return EPropertyType::Output;
		else if (propertyToken == GetPropertyToken(EPropertyType::LocalVariable))
			return EPropertyType::LocalVariable;
		else
			return EPropertyType::Invalid;
	}


	bool IONodeEditorModel::IsPropertyNode(EPropertyType type, const Node* node, std::string_view propertyName)
	{
		if (type == EPropertyType::LocalVariable ? node->Name != propertyName : node->Name != GetPropertyToken(type))
		{
			return false;
		}

		switch (type)
		{
			case EPropertyType::Input: // Graph Input node can only have one output
				return node->Outputs.size() == 1 && node->Outputs[0]->Name == propertyName;
			case EPropertyType::Output: // Graph Output node can only have one input
				return node->Inputs.size() == 1 && node->Inputs[0]->Name == propertyName;
			case EPropertyType::LocalVariable: // Graph Local Variable node can only have one input or output
				return node->Outputs.empty() ? node->Inputs[0]->Name == propertyName : node->Outputs[0]->Name == propertyName;
		}

		return false;
	}


	bool IONodeEditorModel::IsPropertyNode(const Node* node) const
	{
		return (node->Category == GetPropertyToken(EPropertyType::Input)
				&& node->Inputs.empty()
				&& GetPropertySet(EPropertyType::Input).HasValue(node->Outputs[0]->Name))
			|| (node->Category == GetPropertyToken(EPropertyType::Output)
				&& node->Outputs.empty()
				&& GetPropertySet(EPropertyType::Output).HasValue(node->Inputs[0]->Name))
			|| (node->Category == GetPropertyToken(EPropertyType::LocalVariable)
				&& GetPropertySet(EPropertyType::LocalVariable).HasValue(node->Outputs.empty() ? node->Inputs[0]->Name : node->Outputs[0]->Name));
	}


	EPropertyType IONodeEditorModel::GetPropertyTypeOfNode(const Node* node)
	{
		if (!IsPropertyNode(node))
			return EPropertyType::Invalid;

		return GetPropertyType(node->Category);
	}


	bool IONodeEditorModel::IsLocalVariableNode(const Node* node)
	{
		return node->Category == GetPropertyToken(EPropertyType::LocalVariable)
			&& ((node->Inputs.size() == 1 && node->Outputs.size() == 0)
				|| (node->Inputs.size() == 0 && node->Outputs.size() == 1));
	}


	bool IONodeEditorModel::IsSameLocalVariableNodes(const Node* nodeA, const Node* nodeB)
	{
		if (!IsLocalVariableNode(nodeA) || !IsLocalVariableNode(nodeB))
			return false;

		if (&nodeA == &nodeB)
			return true;

		return (nodeA->Inputs.empty() ? nodeA->Outputs[0]->Name : nodeA->Inputs[0]->Name)
			== (nodeB->Inputs.empty() ? nodeB->Outputs[0]->Name : nodeB->Inputs[0]->Name);
	}


	const Pin* IONodeEditorModel::FindLocalVariableSource(std::string_view localVariableName) const
	{
		const Node* setterNode = nullptr;

		// Find setter node for the local variable
		for (auto& node : GetNodes())
		{
			if (IsLocalVariableNode(node) && node->Name == localVariableName && node->Inputs.size() == 1)
			{
				setterNode = node;
				break;
			}
		}

		if (!setterNode)
			return nullptr;

		// find link and pin connected to the input pin of the setter node
		if (auto* link = GetLinkConnectedToPin(setterNode->Inputs[0]->ID))
			return FindPin(link->StartPinID);

		return nullptr;
	}


	Utils::PropertySet& IONodeEditorModel::GetPropertySet(EPropertyType type)
	{
		switch (type)
		{
			case EPropertyType::Input:           return GetInputs();
			case EPropertyType::Output:          return GetOutputs();
			case EPropertyType::LocalVariable:   return GetLocalVariables();
			case EPropertyType::Invalid:
			default:                             ZONG_CORE_ASSERT(false); return GetInputs();
		}
	}


	const Utils::PropertySet& IONodeEditorModel::GetPropertySet(EPropertyType type) const
	{
		switch (type)
		{
			case EPropertyType::Input:           return GetInputs();
			case EPropertyType::Output:          return GetOutputs();
			case EPropertyType::LocalVariable:   return GetLocalVariables();
			case EPropertyType::Invalid:
			default:                             ZONG_CORE_ASSERT(false); return GetInputs();
		}
	}


	std::string IONodeEditorModel::AddPropertyToGraph(EPropertyType type, const choc::value::ValueView& value)
	{
		std::string name;

		if (type == EPropertyType::Invalid)
		{
			ZONG_CORE_ASSERT(false);
			return name;
		}

		const auto getUniqueName = [&](const Utils::PropertySet& propertySet, std::string_view defaultName)
		{
			return Utils::AddSuffixToMakeUnique(std::string(defaultName), [&](const std::string& newName)
			{
				auto properties = propertySet.GetNames();
				return std::any_of(properties.begin(), properties.end(), [&newName](const std::string name) { return name == newName; });
			});
		};

		auto& properties = GetPropertySet(type);

		switch (type)
		{
			case EPropertyType::Input:
				name = getUniqueName(properties, "New Input");
				properties.Set(name, value);
				break;
			case EPropertyType::Output:
				name = getUniqueName(properties, "New Output");
				properties.Set(name, value);
				break;
			case EPropertyType::LocalVariable:
				name = getUniqueName(properties, "New Variable");
				properties.Set(name, value);
				break;
			default:
				break;
		}
		properties.Sort();

		InvalidatePlayer();

		//? DBG
		//ZONG_CORE_WARN("Added Input {} to the graph. Inputs:", name);
		//ZONG_LOG_TRACE(m_GraphInputs.ToJSON());

		return name;
	}


	void IONodeEditorModel::RemovePropertyFromGraph(EPropertyType type, const std::string& name)
	{
		if (type == EPropertyType::Invalid)
		{
			ZONG_CORE_ASSERT(false);
			return;
		}

		auto& properties = GetPropertySet(type);
		properties.Remove(name);

		std::vector<UUID> nodesToDelete;

		for (const auto& node : GetNodes())
		{
			if (IsPropertyNode(type, node, name))
				nodesToDelete.push_back(node->ID);
		}

		RemoveNodes(nodesToDelete);

		InvalidatePlayer();

		//? DBG
		//ZONG_CORE_WARN("Removed Input {} from the graph. Inputs:", name);
	}


	void IONodeEditorModel::SetPropertyValue(EPropertyType type, const std::string& propertyName, const choc::value::ValueView& value)
	{
		if (type == EPropertyType::Invalid)
		{
			ZONG_CORE_ASSERT(false);
			return;
		}

		auto& properties = GetPropertySet(type);
		if (properties.HasValue(propertyName))
		{
			properties.Set(propertyName, value);
			OnGraphPropertyValueChanged(type, propertyName);
		}
	}


	void IONodeEditorModel::RenameProperty(EPropertyType type, const std::string& oldName, const std::string& newName)
	{
		if (type == EPropertyType::Invalid)
		{
			ZONG_CORE_ASSERT(false);
			return;
		}

		auto& properties = GetPropertySet(type);
		if (properties.HasValue(oldName))
		{
			choc::value::Value value = properties.GetValue(oldName);

			properties.Remove(oldName);
			properties.Set(newName, value);
			properties.Sort();

			OnGraphPropertyNameChanged(type, oldName, newName);
		}
	}


	void IONodeEditorModel::ChangePropertyType(EPropertyType type, const std::string& propertyName, const choc::value::ValueView& valueOfNewType)
	{
		if (type == EPropertyType::Invalid)
		{
			ZONG_CORE_ASSERT(false);
			return;
		}

		auto& properties = GetPropertySet(type);
		if (properties.HasValue(propertyName))
		{
			choc::value::Value value = properties.GetValue(propertyName);
			if (value.getType() != valueOfNewType.getType())
			{
				properties.Set(propertyName, valueOfNewType);

				OnGraphPropertyTypeChanged(type, propertyName);
			}
		}
	}


	void IONodeEditorModel::OnGraphPropertyNameChanged(EPropertyType type, const std::string& oldName, const std::string& newName)
	{
		if (type == EPropertyType::Invalid)
			return;

		const auto getPropertyValuePin = [](Node* node, std::string_view propertyName) -> Pin*
		{
			ZONG_CORE_ASSERT(!node->Outputs.empty() || !node->Inputs.empty());

			if (!node->Outputs.empty())
				return node->Outputs[0];
			else if (!node->Inputs.empty())
				return node->Inputs[0];

			return nullptr;
		};

		bool renamed = false;

		for (auto& node : GetNodes())
		{
			if (IsPropertyNode(type, node, oldName))
			{
				if (Pin* pin = getPropertyValuePin(node, oldName))
				{
					pin->Name = newName;

					if (pin->Kind == PinKind::Output)
					{
						if (Link* link = GetLinkConnectedToPin(pin->ID))
						{
							if (Pin* endPin = FindPin(link->EndPinID))
								endPin->Value = choc::value::createString(newName);
						}
					}

					renamed = true;
				}

				if (type == EPropertyType::LocalVariable)
				{
					node->Name = newName;
					renamed = true;
				}
			}
		}

		if (renamed)
			InvalidatePlayer();
	}


	void IONodeEditorModel::OnGraphPropertyTypeChanged(EPropertyType type, const std::string& inputName)
	{
		if (type == EPropertyType::Invalid)
			return;

		auto newProperty = GetPropertySet(type).GetValue(inputName);

		const bool isArray = newProperty.isArray();
		auto [pinType, storageKind] = GetNodeFactory()->GetPinTypeAndStorageKindForValue(newProperty);

		const auto getPropertyValuePin = [](Node* node, std::string_view propertyName) -> Pin**
		{
			ZONG_CORE_ASSERT(!node->Outputs.empty() || !node->Inputs.empty());

			if (!node->Outputs.empty())
				return &node->Outputs[0];
			else if (!node->Inputs.empty())
				return &node->Inputs[0];

			return nullptr;
		};

		bool pinChanged = false;

		for (auto& node : GetNodes())
		{
			if (IsPropertyNode(type, node, inputName))
			{
				if (Pin** pin = getPropertyValuePin(node, inputName))
				{
					// create new pin of the new type
					Pin* newPin = GetNodeFactory()->CreatePinForType(pinType);

					// copy data from the old pin to the new pin
					*newPin = **pin;

					// delete old pin
					delete (*pin);

					// emplace new pin instead of the old pin
					*pin = newPin;

					//pin->Type = pinType;
					newPin->Storage = storageKind;
					newPin->Value = newProperty; // TODO: should use one or the other
					node->Color = GetValueColor(newProperty);

					if (Link* connectedLink = GetLinkConnectedToPin(newPin->ID))
					{
						ax::NodeEditor::DeleteLink(ax::NodeEditor::LinkId(connectedLink->ID));
						RemoveLink(connectedLink->ID);
						// TODO: add visual indication for the user that the node pin has been disconnected, or that the link is invalid
					}

					pinChanged = true;
				}
			}
		}

		if (pinChanged)
			InvalidatePlayer();
	}


	void IONodeEditorModel::OnGraphPropertyValueChanged(EPropertyType type, const std::string& inputName)
	{
		if (type == EPropertyType::Invalid)
			return;

		auto newValue = GetPropertySet(type).GetValue(inputName);
		const auto& valueType = newValue.getType();

		// If wave asset input changed, need to initialize audio data readers for new files
		// which we do when recompiling.
		// TODO: perhaps we could somehow do a clean swap of the readers while the player is playing?
		if (Utils::IsAssetHandle<AssetType::Audio>(valueType) || (valueType.isArray() && Utils::IsAssetHandle<AssetType::Audio>(valueType.getElementType())))
		{
			InvalidatePlayer();
		}

		// TODO (0x): when we have animation graph preview, we will also need to InvalidatePlayer() for other asset types (such as animation clip)

		const auto getPropertyValuePin = [](Node* node, std::string_view propertyName) -> Pin*
		{
			ZONG_CORE_ASSERT(!node->Outputs.empty() || !node->Inputs.empty());

			if (!node->Outputs.empty())
				return node->Outputs[0];
			else if (!node->Inputs.empty())
				return node->Inputs[0];

			return nullptr;
		};

		// Set Outputting value for the "Graph Input" node
		for (auto& node : GetNodes())
		{
			if (IsPropertyNode(type, node, inputName))
			{
				if (Pin* pin = getPropertyValuePin(node, inputName))
					pin->Value = newValue;
			}
		}

		if (type == EPropertyType::LocalVariable)
			InvalidatePlayer();

	}


	// TODO: move this out to some sort of SoundGraph Utils header
	namespace Keyword {
		// TODO: change these for the new SoundGraph
#define KEYWORDS(X) \
			X("if")         X("do")         X("for")        X("let") \
			X("var")        X("int")        X("try")        X("else") \
			X("bool")       X("true")       X("case")       X("enum") \
			X("loop")       X("void")       X("while")      X("break") \
			X("const")      X("int32")      X("int64")      X("float") \
			X("false")      X("using")      X("fixed")      X("graph") \
			X("input")      X("event")      X("class")      X("catch") \
			X("throw")      X("output")     X("return")     X("string") \
			X("struct")     X("import")     X("switch")     X("public") \
			X("double")     X("private")    X("float32")    X("float64") \
			X("default")    X("complex")    X("continue")   X("external") \
			X("operator")   X("processor")  X("namespace")  X("complex32") \
			X("complex64")  X("connection")

		struct Matcher
		{
			static bool Match(std::string_view name) noexcept
			{
#define COMPARE_KEYWORD(str) if (name.length() == (int) sizeof (str) - 1 && choc::text::startsWith(name, (str))) return true;
				KEYWORDS(COMPARE_KEYWORD)
#undef COMPARE_KEYWORD
				return false;
			}
		};
	}


	bool IONodeEditorModel::IsGraphPropertyNameValid(EPropertyType type, std::string_view name) const noexcept
	{
		const auto isPropertyNameValid = [&](const Utils::PropertySet& propertySet)
		{
			return !propertySet.HasValue(name)
				&& !propertySet.HasValue(choc::text::replace(std::string(name), " ", ""))
				&& !Keyword::Matcher::Match(choc::text::replace(std::string(name), " ", ""));
		};

		switch (type)
		{
			case EPropertyType::Invalid:
				return false;
			case EPropertyType::Input:
				return isPropertyNameValid(GetInputs());
			case EPropertyType::Output:
				return isPropertyNameValid(GetOutputs());
			case EPropertyType::LocalVariable:
				return isPropertyNameValid(GetLocalVariables());
			default:
				return false;
		}
	}

	Node* IONodeEditorModel::FindNodeByName(std::string_view name)
	{
		for (auto& node : GetNodes())
		{
			if (node->Name == name)
				return node;
		}
		return nullptr;
	}


	NodeEditorModel::LinkQueryResult IONodeEditorModel::CanCreateLink(Pin * startPin, Pin * endPin)
	{
		LinkQueryResult result = NodeEditorModel::CanCreateLink(startPin, endPin);

		if (!result)
			return result;

		auto startNode = FindNode(startPin->NodeID);
		auto endNode = FindNode(endPin->NodeID);

		if (!startNode) return LinkQueryResult::InvalidStartPin;
		if (!endNode) return LinkQueryResult::InvalidEndPin;

		// TODO: in the future might need to change this to check if the nodes are from the same LV
		if (IsLocalVariableNode(startNode) && IsLocalVariableNode(endNode))
		{
			if (startPin->Name == endPin->Name)
				return LinkQueryResult::CausesLoop;
		}

		return LinkQueryResult::CanConnect;
	}


	void IONodeEditorModel::OnNodeCreated()
	{
		InvalidatePlayer();

		if (onNodeCreated)
			onNodeCreated();
	}

	void IONodeEditorModel::OnNodeDeleted()
	{
		InvalidatePlayer();

		if (onNodeDeleted)
			onNodeDeleted();
	}

	void IONodeEditorModel::OnLinkCreated(UUID linkID)
	{
		if (auto* newLink = FindLink(linkID))
		{
			auto* startPin = FindPin(newLink->StartPinID);
			auto* endPin = FindPin(newLink->EndPinID);
			ZONG_CORE_ASSERT(endPin);

			auto* startNode = FindNode(startPin->NodeID);
			ZONG_CORE_ASSERT(startNode);

			auto* endNode = FindNode(endPin->NodeID);
			ZONG_CORE_ASSERT(startPin && endNode);

			//? JP. no idea why we needed this :shrug:
#if 0
			// If pin is connected to Graph Input getter node,
			// set its name as the value for this pin
			const bool isGraphInput = startNode->Category == GetPropertyToken(EPropertyType::Input);
			if (isGraphInput && GetInputs().HasValue(startPin->Name))
			{
				endPin->Value = choc::value::Value(startPin->Name);
				if (onPinValueChanged)
					onPinValueChanged(endNode->ID, endPin->ID);
			}
#endif

			// TODO: handle connections to Local Variables?
			//else if (m_GraphAsset->LocalVariables.HasValue(startPin->Name))

			// Remove any other links connected to the same input pin
			// For Sound Graph nodes we only allow single input connection,
			// but multiple output connections.
			std::vector<UUID> linkIds = GetAllLinkIDsConnectedToPin(newLink->EndPinID);
			Utils::Remove(linkIds, newLink->ID);
			RemoveLinks(linkIds);
		}

		InvalidatePlayer();

		if (onLinkCreated)
			onLinkCreated(linkID);
	}

	void IONodeEditorModel::OnLinkDeleted()
	{
		InvalidatePlayer();

		if (onLinkDeleted)
			onLinkDeleted();
	}

} // namespace Engine
