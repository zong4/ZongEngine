#pragma once

#include "AnimationGraphNodes.h"
#include "AnimationGraphAsset.h"

#include "Engine/Editor/NodeGraphEditor/NodeEditorModel.h"
#include "Engine/Editor/NodeGraphEditor/NodeGraphUtils.h"
#include "Engine/Editor/NodeGraphEditor/PropertySet.h"

namespace Hazel {

	namespace AnimationGraph {

		// Stores mapping of id to name
		// Needed only so that we can display the names of animation graph inputs in the scene hierarchy panel.
		// Not needed during runtime
		void RegisterIdentifier(Identifier id, std::string_view name);

		const std::string& GetName(Identifier id);

	}


	class AnimationGraphNodeEditorModel : public IONodeEditorModel
	{
	public:
		AnimationGraphNodeEditorModel(Ref<AnimationGraphAsset> AnimationGraph);

		~AnimationGraphNodeEditorModel() = default;

		AssetHandle GetSkeletonHandle() const;
		void SetSkeletonHandle(AssetHandle handle);

		// Ensure that given parent node has a sub-graph (create one if necessary)
		void EnsureSubGraph(Node* parent);

		// Ensure that every state machine in the graph has an entry state defined
		void EnsureEntryStates();

		// Returns false if the node probably will not compile
		// This does not cover 100% of all things that can go wrong, but we can check
		// some for some of the more obvious problems.
		bool IsWellDefined(Node* node);

		Node* CreateTransitionNode();

		Node* FindInAllNodes(UUID id);
		Node* FindParent(UUID id);

		std::unordered_map<UUID, std::vector<Node*>>& GetAllNodes() { return m_AnimationGraph->m_Nodes; }
		std::vector<Node*>& GetNodes() override { return m_AnimationGraph->m_Nodes.at(m_RootNodeId); }
		const std::vector<Node*>& GetNodes() const override { return m_AnimationGraph->m_Nodes.at(m_RootNodeId); }
		Node* GetNode(uint32_t index) { return GetNodes().at(index); }
		void SetNodes(std::vector<Node*> nodes) { m_AnimationGraph->m_Nodes[m_RootNodeId] = std::move(nodes); }

		std::unordered_map<UUID, std::vector<Link>>& GetAllLinks() { return m_AnimationGraph->m_Links; }
		std::vector<Link>& GetLinks() override { return m_AnimationGraph->m_Links.at(m_RootNodeId); }
		const std::vector<Link>& GetLinks() const override { return m_AnimationGraph->m_Links.at(m_RootNodeId); }
		void SetLinks(std::vector<Link> links) { m_AnimationGraph->m_Links[m_RootNodeId] = std::move(links); }

		const std::vector<std::string>& GetInputTypes() const override { static std::vector<std::string> types = { "AnimationAsset", "Bool", "Float", "Int" }; return types; }

		Utils::PropertySet& GetInputs() override { return m_AnimationGraph->Inputs; }
		Utils::PropertySet& GetOutputs() override { return m_AnimationGraph->Outputs; }
		Utils::PropertySet& GetLocalVariables() override { return m_AnimationGraph->LocalVariables; }

		const Utils::PropertySet& GetInputs() const override { return m_AnimationGraph->Inputs; }
		const Utils::PropertySet& GetOutputs() const override { return m_AnimationGraph->Outputs; }
		const Utils::PropertySet& GetLocalVariables() const override { return m_AnimationGraph->LocalVariables; }

		const Nodes::Registry& GetNodeTypes() const override;

		bool SaveGraphState(const char* data, size_t size) override;
		const std::string& LoadGraphState() override;
		std::unordered_map<UUID, std::string>& GetAllGraphStates() { return m_AnimationGraph->m_GraphState; }

		Ref<AnimationGraphAsset> GetAnimationGraph() const { return m_AnimationGraph; }

		UUID SetRootNodeID(UUID id) { UUID previousRootId = m_RootNodeId; m_RootNodeId = id; return previousRootId; }
		const std::vector<Node*>& GetCurrentPath() const { return m_CurrentPath; }
		Node* SetCurrentPath(Node* node);
		void InvalidateGraphState(bool invalidate = true) { m_IsGraphStateValid = !invalidate; }
		bool IsGraphStateValid() { return m_IsGraphStateValid; }
		void InvalidateSubGraphState(bool invalidate = true) { m_IsSubGraphStateValid = !invalidate; }
		bool IsSubGraphStateValid() { return m_IsSubGraphStateValid; }

		void RemovePropertyFromGraph(EPropertyType type, const std::string& name) override;
		void OnGraphPropertyNameChanged(EPropertyType type, const std::string& oldName, const std::string& newName) override;
		void OnGraphPropertyTypeChanged(EPropertyType type, const std::string& inputName) override;
		void OnGraphPropertyValueChanged(EPropertyType type, const std::string& inputName) override;

		Node* SpawnGraphInputNode(const std::string& inputName) override;
		Node* SpawnLocalVariableNode(const std::string& inputName, bool getter) override;

	protected:
		bool Sort() override;

		void Serialize() override;;

		void Deserialize() override;

		const Nodes::AbstractFactory* GetNodeFactory() const override;

		void AssignSomeDefaultValue(Pin* pin) override;

		void OnCompileGraph() override;

		// When root node is a state machine, ensure there is an entry state
		void EnsureEntryState();

	private:
		AnimationGraph::AnimationGraphNodeFactory m_AnimationGraphNodeFactory;
		AnimationGraph::StateMachineNodeFactory m_StateMachineNodeFactory;
		AnimationGraph::TransitionGraphNodeFactory m_TransitionGraphNodeFactory;
		Ref<AnimationGraphAsset> m_AnimationGraph = nullptr;
		std::vector<Node*> m_CurrentPath;
		UUID m_RootNodeId = 0;
		bool m_IsGraphStateValid = true;
		bool m_IsSubGraphStateValid = true;
	};

}
