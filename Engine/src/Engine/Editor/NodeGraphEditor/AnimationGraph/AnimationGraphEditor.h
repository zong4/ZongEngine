#pragma once

#include "AnimationGraphNodeEditorModel.h"

#include "Engine/Editor/NodeGraphEditor/NodeGraphEditor.h"
#include "Engine/Editor/NodeGraphEditor/AnimationGraph/AnimationGraphEditorTypes.h"

#include <vector>

namespace Engine {

	class AnimationGraphEditor : public IONodeGraphEditor
	{
	public:
		AnimationGraphEditor();

		virtual void SetAsset(const Ref<Asset>& asset) override;

		//static std::pair<AnimationGraph::Types::EPinType, StorageKind> GetPinTypeAndStorageKindForValue(const choc::value::Value& v);

	protected:
		bool InitializeEditor() override;
		void OnOpen() override;
		void OnClose() override;

		void OnRenderOnCanvas(ImVec2 min, ImVec2 max) override;
		void OnRender() override;

		void DrawNodes(PinPropertyContext& pinContext) override;
		void DrawStateNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext);
		void DrawNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext) override;

		void DrawGraphIO() override;

		bool DrawPinPropertyEdit(PinPropertyContext& context) override;
		void DrawPinIcon(const Pin* pin, bool connected, int alpha) override;

		// Track selection of custom-drawn transition nodes
		void SelectTransition(UUID id, bool append = false) { if (append) m_SelectedTransitionNodes.emplace(id); else m_SelectedTransitionNodes = { id }; static_cast<AnimationGraphNodeEditorModel*>(GetModel())->InvalidateSubGraphState(); }
		void ClearSelectedTransitions() { m_SelectedTransitionNodes.clear(); }
		bool IsAnyTransitionSelected() const { return !m_SelectedTransitionNodes.empty(); }
		bool IsTransitionSelected(UUID id) const { return std::find(m_SelectedTransitionNodes.begin(), m_SelectedTransitionNodes.end(), id) != m_SelectedTransitionNodes.end(); }

		Node* CreateTransition(UUID startNodeID, UUID endNodeID);

		void DeleteSelection();
		void CopySelection();
		std::vector<UUID> PasteSelection(const std::vector<UUID>& selectedNodeIDs, ImVec2 position);

		void DrawNodeContextMenu(Node* node) override;


	private:
		ax::NodeEditor::EditorContext* m_SubGraph = nullptr; // lifetime controlled by imgui_node_editor
		std::unordered_set<UUID> m_SelectedTransitionNodes;
		std::unordered_set<UUID> m_TransitionsToDelete;

		std::function<Node* (bool searching, std::string_view searchedString)> baseOnNodeListPopup = nullptr;

		std::vector<UUID> m_CopiedNodeIDs;
	};

}
