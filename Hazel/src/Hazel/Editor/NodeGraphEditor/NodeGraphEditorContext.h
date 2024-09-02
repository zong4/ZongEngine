#pragma once

// This file exists to avoid having to include imgui_node_editor.h in NodeGraphEditor.h

#include "NodeGraphEditor.h"
#include "imgui-node-editor/imgui_node_editor.h"

namespace Hazel {

	struct NodeGraphEditorBase::ContextState
	{
		bool CreateNewNode = false;
		bool NewNodePopupOpening = false;
		UUID NewNodeLinkPinId = 0;
		Pin* NewLinkPin = nullptr;
		bool DraggingInputField = false;

		UUID EditNodeId = 0;

		ax::NodeEditor::NodeId ContextNodeId = 0;
		ax::NodeEditor::LinkId ContextLinkId = 0;
		ax::NodeEditor::PinId ContextPinId = 0;

		// For state machine graphs
		UUID TransitionStartNode = 0;
		UUID HoveredNode = 0;
		UUID HoveredTransition = 0;
		ImVec2 LastTransitionEndPoint = { 0.0f, 0.0f };
	};

}
