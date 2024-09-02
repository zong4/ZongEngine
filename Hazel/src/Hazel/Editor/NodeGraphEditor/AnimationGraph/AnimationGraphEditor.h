#pragma once

#include "AnimationGraphNodeEditorModel.h"

#include "Hazel/Editor/NodeGraphEditor/NodeGraphEditor.h"
#include "Hazel/Editor/NodeGraphEditor/AnimationGraph/AnimationGraphEditorTypes.h"

#include <vector>

namespace Hazel {

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
		void DrawNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext) override;
		void DrawNodeForStateMachine(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext);
		void DrawStateNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext);
		void DrawNodeForBlendSpace(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext);
		void DrawBlendSpaceTriangles();


		void DrawGraphIO() override;

		bool PropertyBone(Pin* pin, bool& openBonePopup);
		bool DrawPinPropertyEdit(PinPropertyContext& context) override;
		void DrawPinIcon(const Pin* pin, bool connected, int alpha) override;

		// Track selection of custom-drawn transition nodes
		void SelectTransition(UUID id, bool append = false) { if (append) m_SelectedTransitionNodes.emplace(id); else m_SelectedTransitionNodes = { id }; static_cast<AnimationGraphNodeEditorModel*>(GetModel())->InvalidateSubGraphState(); }
		void ClearSelectedTransitions() { m_SelectedTransitionNodes.clear(); }
		bool IsAnyTransitionSelected() const { return !m_SelectedTransitionNodes.empty(); }
		bool IsTransitionSelected(UUID id) const { return std::find(m_SelectedTransitionNodes.begin(), m_SelectedTransitionNodes.end(), id) != m_SelectedTransitionNodes.end(); }

		Node* CreateTransition(UUID startNodeID, UUID endNodeID);

		// Track selection of custom-drawn blend space vertices
		void SelectBlendSpaceVertex(UUID id, bool append = false) { if (append) m_SelectedBlendSpaceVertices.emplace(id); else m_SelectedBlendSpaceVertices = { id }; static_cast<AnimationGraphNodeEditorModel*>(GetModel())->InvalidateSubGraphState(); }
		void ClearSelectedBlendSpaceVertices() { m_SelectedBlendSpaceVertices.clear(); }
		bool IsAnyBlendSpaceVertexSelected() const { return !m_SelectedBlendSpaceVertices.empty(); }
		bool IsBlendSpaceVertexSelected(UUID id) const { return std::find(m_SelectedBlendSpaceVertices.begin(), m_SelectedBlendSpaceVertices.end(), id) != m_SelectedBlendSpaceVertices.end(); }

		void DeleteSelection();
		void CopySelection();
		std::vector<UUID> PasteSelection(const std::vector<UUID>& selectedNodeIDs, ImVec2 position);

		void CheckContextMenus() override;
		void DrawNodeContextMenu(Node* node) override;
		void DrawBackgroundContextMenu(bool& isNewNodePoppedUp, ImVec2& newNodePosition) override;
		void DrawDeferredComboBoxes(PinPropertyContext& pinContext) override;

	private:
		constexpr static float s_BlendSpaceVertexRadius = 5.0f;

		ax::NodeEditor::EditorContext* m_SubGraph = nullptr; // lifetime controlled by imgui_node_editor
		std::unordered_set<UUID> m_SelectedTransitionNodes;
		std::unordered_set<UUID> m_TransitionsToDelete;

		std::unordered_set<UUID> m_SelectedBlendSpaceVertices;
		std::unordered_set<UUID> m_BlendSpaceVerticesToDelete;

		std::function<Node* (bool searching, std::string_view searchedString)> baseOnNodeListPopup = nullptr;

		std::vector<UUID> m_CopiedNodeIDs;
	};

}
