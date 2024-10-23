#pragma once

#include "Nodes.h"
#include "PropertySet.h"

#include "Engine/Asset/AssetImporter.h"

#include <choc/containers/choc_Value.h>

#include <functional>

namespace Hazel {

	class NodeEditorModel
	{
	public:
		NodeEditorModel(const NodeEditorModel&) = delete;
		NodeEditorModel& operator=(const NodeEditorModel&) = delete;

		NodeEditorModel() = default;
		virtual ~NodeEditorModel() = default;

		//==================================================================================
		/// Graph Model Interface
		virtual std::vector<Node*>& GetNodes() = 0;
		virtual std::vector<Link>& GetLinks() = 0;
		virtual const std::vector<Node*>& GetNodes() const = 0;
		virtual const std::vector<Link>& GetLinks() const = 0;

		virtual const Nodes::Registry& GetNodeTypes() const = 0;

		//==================================================================================
		/// Graph Model
		uintptr_t GetIDFromString(const std::string& idString) const;

		ImColor GetIconColor(int pinTypeID) const { return GetNodeFactory()->GetIconColor(pinTypeID); }
		ImColor GetValueColor(const choc::value::Value& v) const { return GetNodeFactory()->GetValueColor(v); }

		Node* FindNode(UUID id);
		Link* FindLink(UUID id);
		Pin* FindPin(UUID id);
		const Node* FindNode(UUID id) const;
		const Link* FindLink(UUID id) const;
		const Pin* FindPin(UUID id)   const;

		Link* GetLinkConnectedToPin(UUID pinID);
		const Link* GetLinkConnectedToPin(UUID pinID) const;
		const Node* GetNodeForPin(UUID pinID) const;
		const Node* GetNodeConnectedToPin(UUID pinID) const;	// Get node on the other side of the link
		Node* GetNodeConnectedToPin(UUID pinID);				// Get node on the other side of the link
		bool IsPinLinked(UUID pinID);
		bool IsPinLinked(UUID pinID) const;

		std::vector<Link*> GetAllLinksConnectedToPin(UUID pinID);
		std::vector<UUID> GetAllLinkIDsConnectedToPin(UUID pinID);

		struct LinkQueryResult
		{
			enum
			{
				CanConnect,
				InvalidStartPin,
				InvalidEndPin,
				IncompatiblePinKind,
				IncompatibleStorageKind,
				IncompatibleType,
				SamePin,
				SameNode,
				CausesLoop
			} Reason;

			LinkQueryResult(decltype(Reason) value) : Reason(value) {}

			explicit operator bool()
			{
				return Reason == LinkQueryResult::CanConnect;
			}
		};
		virtual LinkQueryResult CanCreateLink(Pin* startPin, Pin* endPin);
		void CreateLink(Pin* startPin, Pin* endPin);
		void RemoveLink(UUID linkId);
		void RemoveLinks(const std::vector<UUID>& linkIds);

		Node* CreateNode(const std::string& category, const std::string& typeID);
		void  RemoveNode(UUID nodeId);
		void  RemoveNodes(const std::vector<UUID>& nodeIds);

		void SaveAll();
		void LoadAll();
		virtual bool SaveGraphState(const char* data, size_t size) = 0;
		virtual const std::string& LoadGraphState() = 0;

		void CompileGraph();

	private:
		//==================================================================================
		/// Internal
		void AssignPinIds(Node* node);
		void DeleteDeadLinks(UUID nodeId);

		// Used when deleting links to reassign some sort of valid default value
		virtual void AssignSomeDefaultValue(Pin* pin);
		
	protected:
		// Adds node to the list of nodes
		void OnNodeSpawned(Node* node);

		// Sort nodes in order that makes sense
		virtual bool Sort() { return false; }

		//==================================================================================
		/// Graph Model Interface
	public:
		using NodeID = UUID;
		using PinID = UUID;
		using LinkID = UUID;

		std::function<void()> onNodeCreated = nullptr;
		std::function<void()> onNodeDeleted = nullptr;
		std::function<void(UUID linkID)> onLinkCreated = nullptr;
		std::function<void()> onLinkDeleted = nullptr;

		std::function<void(NodeID, PinID)> onPinValueChanged = nullptr;

		std::function<void(Node* node)> onEditNode = nullptr;
		
		std::function<bool(Ref<Asset> graphAsset)> onCompile = nullptr;
		std::function<void()> onCompiledSuccessfully = nullptr;
		std::function<void()> onPlay = nullptr;
		std::function<void(bool onClose)> onStop = nullptr;

	protected:
		virtual void OnNodeCreated() { if (onNodeCreated) onNodeCreated(); }
		virtual void OnNodeDeleted() { if (onNodeDeleted) onNodeDeleted(); }
		virtual void OnLinkCreated(UUID linkID) { if (onLinkCreated) onLinkCreated(linkID); }
		virtual void OnLinkDeleted() { if (onLinkDeleted) onLinkDeleted(); }

		virtual void OnCompileGraph() {};

		virtual const Nodes::AbstractFactory* GetNodeFactory() const = 0;

	private:
		virtual void Serialize() = 0;
		virtual void Deserialize() = 0;
	};


	inline bool operator==(NodeEditorModel::LinkQueryResult a, NodeEditorModel::LinkQueryResult b)
	{
		return a.Reason == b.Reason;
	}


	inline bool operator!=(NodeEditorModel::LinkQueryResult a, NodeEditorModel::LinkQueryResult b)
	{
		return !(a == b);
	}


	// Extend node editor model with concept of "Inputs" and "Outputs"
	// Used for both Animation and Sound graphs
	class IONodeEditorModel : public NodeEditorModel
	{
	public:
		// names of types of graph input. e.g. { "Float", "Int", "Bool", "String", "WaveAsset" }
		virtual const std::vector<std::string>& GetInputTypes() const = 0;

		virtual Utils::PropertySet& GetInputs() = 0;
		virtual Utils::PropertySet& GetOutputs() = 0;
		virtual Utils::PropertySet& GetLocalVariables() = 0;
		virtual const Utils::PropertySet& GetInputs() const = 0;
		virtual const Utils::PropertySet& GetOutputs() const = 0;
		virtual const Utils::PropertySet& GetLocalVariables() const = 0;

		static std::string GetPropertyToken(EPropertyType type);
		static EPropertyType GetPropertyType(std::string_view propertyToken);
		static bool IsPropertyNode(EPropertyType type, const Node* node, std::string_view propertyName);
		static bool IsLocalVariableNode(const Node* node);
		static bool IsSameLocalVariableNodes(const Node* nodeA, const Node* nodeB);
		const Pin* FindLocalVariableSource(std::string_view localVariableName) const;

		EPropertyType GetPropertyTypeOfNode(const Node* node);
		/** @returns true if any of the property sets contain property with the name of the node or its out/in pin. */
		bool IsPropertyNode(const Node* node) const;

		virtual Utils::PropertySet& GetPropertySet(EPropertyType type);
		virtual const Utils::PropertySet& GetPropertySet(EPropertyType type) const;

		virtual std::string AddPropertyToGraph(EPropertyType type, const choc::value::ValueView& value);
		virtual void RemovePropertyFromGraph(EPropertyType type, const std::string& name);
		virtual void SetPropertyValue(EPropertyType type, const std::string& propertyName, const choc::value::ValueView& value);
		virtual void RenameProperty(EPropertyType type, const std::string& oldName, const std::string& newName);
		virtual void ChangePropertyType(EPropertyType type, const std::string& propertyName, const choc::value::ValueView& valueOfNewType);

		virtual void OnGraphPropertyNameChanged(EPropertyType type, const std::string& oldName, const std::string& newName);
		virtual void OnGraphPropertyTypeChanged(EPropertyType type, const std::string& inputName);
		virtual void OnGraphPropertyValueChanged(EPropertyType type, const std::string& inputName);

		// TODO: SpawnGraphOutputNode
		virtual Node* SpawnGraphInputNode(const std::string& inputName) = 0;
		virtual Node* SpawnLocalVariableNode(const std::string& inputName, bool getter) = 0;

		bool IsPlayerDirty() const noexcept { return m_PlayerDirty; }

		// Flag player dirty. Dirty player needs to be recompiled
		// and parameters changes can't be applied live.
		virtual void InvalidatePlayer(const bool isDirty = true) { m_PlayerDirty = isDirty; }

		virtual bool IsGraphPropertyNameValid(EPropertyType type, std::string_view name) const noexcept;

	protected:
		Node* FindNodeByName(std::string_view name);

		LinkQueryResult CanCreateLink(Pin* startPin, Pin* endPin) override;

		void OnNodeCreated() override;
		void OnNodeDeleted() override;
		void OnLinkCreated(UUID linkID) override;
		void OnLinkDeleted() override;

	private:
		bool m_PlayerDirty = true;
	};

} // namespace Hazel

