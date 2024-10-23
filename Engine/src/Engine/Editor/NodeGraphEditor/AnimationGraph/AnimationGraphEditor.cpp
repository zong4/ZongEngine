#include "pch.h"
#include "AnimationGraphEditor.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Editor/NodeGraphEditor/NodeGraphEditorContext.h"
#include "Engine/Vendor/imgui-node-editor/widgets.h"

#include <choc/text/choc_StringUtilities.h>
#include <imgui-node-editor/builders.h>
#include <imgui-node-editor/imgui_node_editor_internal.h>

namespace ed = ax::NodeEditor;
namespace AG = Hazel::AnimationGraph;

static ax::NodeEditor::Detail::EditorContext* GetEditorContext()
{
	return (ax::NodeEditor::Detail::EditorContext*)(ed::GetCurrentEditor());
}

namespace Hazel {

	AnimationGraphEditor::AnimationGraphEditor() : IONodeGraphEditor("Animation Graph Editor")
	{
		m_NodeBuilderTexture = EditorResources::TranslucentTexture;

		onDragDropOntoCanvas = [&]() {
			// Only state machine sub-graphs accept a dropped asset
			auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
			auto currentPath = model->GetCurrentPath();
			if (currentPath.empty() || (currentPath.back()->GetTypeID() != AG::Types::ENodeType::StateMachine))
			{
				return;
			}

			auto data = ImGui::AcceptDragDropPayload("asset_payload");
			if (data)
			{
				AssetHandle assetHandle = *(AssetHandle*)data->Data;
				Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
				if (asset && asset->GetAssetType() == AssetType::Animation)
				{
					auto* node = model->CreateNode("StateMachine", "Quick State");
					ZONG_CORE_ASSERT(node);
					ed::SetNodePosition(ed::NodeId(node->ID), ed::ScreenToCanvas(ImGui::GetMousePos()));
					node->Inputs[0]->Value = Utils::CreateAssetHandleValue<AssetType::Animation>(assetHandle);
					if(GetModel()->onPinValueChanged)
						GetModel()->onPinValueChanged(node->ID, node->Inputs[0]->ID);
				}
			}
		};

		baseOnNodeListPopup = onNodeListPopup;
		onNodeListPopup = [&](bool searching, std::string_view searchedString) -> Node* {
			auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
			auto currentPath = model->GetCurrentPath();

			ed::ClearSelection();

			if (!m_CopiedNodeIDs.empty())
			{
				if (ImGui::MenuItem("Paste"))
				{
					auto newNodeIds = PasteSelection(m_CopiedNodeIDs, ed::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup()));
					return newNodeIds.empty() ? nullptr : model->FindNode(newNodeIds.front());
				}
				ImGui::Separator();
			}

			if (currentPath.empty() || (currentPath.back()->GetTypeID() != AG::Types::ENodeType::StateMachine))
			{
				// baseOnNodeListPopup adds graph inputs to the menu.
				// We don't want that for state machines
				return baseOnNodeListPopup(searching, searchedString);
			}
			return nullptr;
		};

		onNewNodeFromPopup = [&](Node* node) {
			auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
			auto currentPath = model->GetCurrentPath();
			if (currentPath.empty() || (currentPath.back()->GetTypeID() != AG::Types::ENodeType::StateMachine))
			{
				return;
			}

			if (m_State->TransitionStartNode != 0 && (node->GetTypeID() == AG::Types::ENodeType::StateMachine || node->GetTypeID() == AG::Types::ENodeType::State || node->GetTypeID() == AG::Types::ENodeType::QuickState))
			{
				CreateTransition(m_State->TransitionStartNode, node->ID);
			}

			m_State->TransitionStartNode = 0;
		};

		onNewNodePopupCancelled = [&]() {
			m_State->TransitionStartNode = 0;
		};

	}


	void AnimationGraphEditor::SetAsset(const Ref<Asset>& asset)
	{
		// Load a copy of asset.
		// This allows us to "compile" the asset (e.g. press "Compile" button in the UI) without affecting the asset in AssetManager
		// This could facilitate (later) user closing the animation graph editor without saving changes (e.g. if they are not happy with compiled result)
		auto metadata = Project::GetEditorAssetManager()->GetMetadata(asset);
		Ref<Asset> assetCopy;
		AssetImporter::TryLoadData(metadata, assetCopy);
		auto AnimationGraph = assetCopy.As<AnimationGraphAsset>();
		if (!AnimationGraph)
		{
			SetOpen(false);
			return;
		}
		IONodeGraphEditor::SetModel(std::make_unique<AnimationGraphNodeEditorModel>(AnimationGraph));
		NodeGraphEditorBase::SetAsset(assetCopy);
	}


	void AnimationGraphEditor::OnRenderOnCanvas(ImVec2 min, ImVec2 max)
	{
		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
		const ImVec2 linkPos({ 20.0f, 20.0f });
		ImGui::SetCursorPos(linkPos);

		if (UI::Hyperlink(GetTitle().c_str()))
		{
			model->SetCurrentPath(nullptr);
			model->InvalidateGraphState();
			return;
		}
		for (auto node : model->GetCurrentPath())
		{
			ImGui::SameLine();
			ImGui::TextUnformatted("/");
			ImGui::SameLine();
			if (UI::Hyperlink(node->Description.c_str()))
			{
				model->SetCurrentPath(node);
				model->InvalidateGraphState();
				return;
			}
		}

		if (!model->GetCurrentPath().empty() && (model->GetCurrentPath().back()->GetTypeID() == AG::Types::ENodeType::StateMachine))
		{
			UI::ScopedFont normalFont(ImGui::GetIO().Fonts->Fonts[0]);
			UI::ScopedColour darkText(ImGuiCol_Text, ImColor(255, 255, 255, 60));
			UI::ShiftCursor(20.0f, 20.0f);
			ImGui::TextUnformatted("Double click to edit a state.");
			UI::ShiftCursorX(20.0f);
			ImGui::TextUnformatted("ALT + click and drag to create transitions.");
		}

	}


	void AnimationGraphEditor::OnRender()
	{
		IONodeGraphEditor::OnRender();

		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		auto currentPath = model->GetCurrentPath();
		if (!currentPath.empty() && (currentPath.back()->GetTypeID() == AG::Types::ENodeType::StateMachine))
		{
			ImGui::SetNextWindowClass(GetWindowClass());
			if (ImGui::Begin("Subgraph", nullptr, ImGuiWindowFlags_NoCollapse))
			{
				if (!m_FirstFrame)
				{
					ed::SetCurrentEditor(m_SubGraph);
					if (m_SelectedTransitionNodes.size() == 1)
					{
						auto currentTail = currentPath.back();
						auto selectedTransition = model->FindNode(*m_SelectedTransitionNodes.begin());
						ZONG_CORE_ASSERT(selectedTransition);
						//EnsureWindowIsDocked(ImGui::GetCurrentWindow());
						model->SetCurrentPath(selectedTransition);
						DrawGraph("Subgraph");
						if (ed::HasSelectionChanged())
						{
							std::vector<UUID> selectedNodeIDs = GetSelectedNodeIDs();
							if (selectedNodeIDs.size() == 1)
							{
								SelectNode(selectedNodeIDs.back());
							}
							else if (IsAnyNodeSelected())
							{
								ClearSelection();
							}
						}
						model->SetCurrentPath(currentTail);

						auto startNode = model->GetNodeConnectedToPin(selectedTransition->Inputs[0]->ID);
						auto endNode = model->GetNodeConnectedToPin(selectedTransition->Outputs[0]->ID);
						ZONG_CORE_ASSERT(startNode && endNode);
						{
							UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
							UI::ScopedColour darkText(ImGuiCol_Text, ImColor(255, 255, 255, 60));
							ImGui::SetCursorPos({ 20.0f, 20.0f });
							ImGui::Text("Transition: %s to %s", startNode->Description.c_str(), endNode->Description.c_str());
						}
					}
					else
					{
						// Either no transition is selected, or multiple transitions are selected.
						// Draw an empty canvas (with shadow effect)
						ed::Begin("Subgraph");
						ed::End();
						auto editorMin = ImGui::GetItemRectMin();
						auto editorMax = ImGui::GetItemRectMax();
						UI::DrawShadowInner(EditorResources::ShadowTexture, 50, editorMin, editorMax, 0.3f, (editorMax.y - editorMin.y) / 4.0f);
						{
							UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
							UI::ScopedColour darkText(ImGuiCol_Text, ImColor(255, 255, 255, 60));
							ImGui::SetCursorPos({ 20.0f, 20.0f });
							ImGui::TextUnformatted("No transition (or multiple) selected");
						}
					}
					ed::SetCurrentEditor(m_Editor);
				}
			}
			ImGui::End(); // Subgraph
		}
	}


	bool AnimationGraphEditor::InitializeEditor()
	{
		if (IONodeGraphEditor::InitializeEditor())
		{
			m_SubGraph = ed::CreateEditor(&GetEditorContext()->GetConfig());
			ed::SetCurrentEditor(m_SubGraph);
			InitializeEditorStyle(ed::GetStyle());
			ed::SetCurrentEditor(m_Editor);
			return true;
		}
		return false;
	}


	void AnimationGraphEditor::OnOpen()
	{
		if (!GetModel())
		{
			SetOpen(false);
			return;
		}
	}

	void AnimationGraphEditor::OnClose()
	{
		m_CopiedNodeIDs.clear();
		m_SelectedTransitionNodes.clear();

		ZONG_CORE_ASSERT(ed::GetCurrentEditor() == m_Editor);

		if (m_SubGraph)
		{
			// Destroy editor causes settings to be saved.
			// However, saving the settings at this point results in bung settings.
			// (probably because m_Subgraph is not the current editor context at this point)
			// Disable saving.
			// (the settings are saved every time the canvas is panned or zoomed anyway, so 
			// this is not missing out anything)
			//
			// imgui_node_editor api does not make this easy...
			ed::SetCurrentEditor(m_SubGraph);
			const_cast<ax::NodeEditor::Detail::Config&>(GetEditorContext()->GetConfig()).SaveSettings = nullptr;

			ed::DestroyEditor(m_SubGraph);
			m_SubGraph = nullptr;
		}

		// TODO (0x): ask user if they want to save changes and allow cancel.  For now, it always saves.
		IONodeGraphEditor::OnClose();
	}


	ImVec2 GetClosestPointOnRectBorder(const ImRect& rect, const ImVec2& point)
	{
		ImVec2 const points[4] =
		{
			ImLineClosestPoint(rect.GetTL(), rect.GetTR(), point),
			ImLineClosestPoint(rect.GetBL(), rect.GetBR(), point),
			ImLineClosestPoint(rect.GetTL(), rect.GetBL(), point),
			ImLineClosestPoint(rect.GetTR(), rect.GetBR(), point)
		};

		float distancesSq[4] =
		{
			ImLengthSqr(points[0] - point),
			ImLengthSqr(points[1] - point),
			ImLengthSqr(points[2] - point),
			ImLengthSqr(points[3] - point)
		};

		float lowestDistance = FLT_MAX;
		int32_t closestPointIdx = -1;
		for (auto i = 0; i < 4; i++)
		{
			if (distancesSq[i] < lowestDistance)
			{
				closestPointIdx = i;
				lowestDistance = distancesSq[i];
			}
		}

		return points[closestPointIdx];
	}


	ImVec2 GetConnectionPointBetweenRectAndPoint(const ImRect& rect, const ImVec2& endPoint)
	{
		ImVec2 startPoint = rect.GetCenter();
		ImVec2 const midPoint = startPoint + ((endPoint - startPoint) / 2);

		return GetClosestPointOnRectBorder(rect, midPoint);
	}


	void GetConnectionPointsBetweenRects(const ImRect& startRect, const ImRect& endRect, ImVec2& startPoint, ImVec2& endPoint)
	{
		startPoint = startRect.GetCenter();
		endPoint = endRect.GetCenter();
		ImVec2 const midPoint = startPoint + ((endPoint - startPoint) / 2);

		startPoint = GetClosestPointOnRectBorder(startRect, midPoint);
		endPoint = GetClosestPointOnRectBorder(endRect, midPoint);
	}


	void DrawOffsetArrow(ImVec2 startPoint, ImVec2 endPoint, const float offset, const float lineWidth, const ImVec2& arrowHeadSize, ImU32 color)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		ImVec2 arrowDir = ImVec2(endPoint - startPoint);
		float arrowLength = sqrt(ImLengthSqr(arrowDir));
		if (arrowLength > 0.0)
		{
			arrowDir /= arrowLength;
		}
		ImVec2 orthogonal = { arrowDir.y, -arrowDir.x };
		ImVec2 offsetVec = (orthogonal * offset);

		startPoint += offsetVec;
		endPoint += offsetVec;
		ImVec2 arrowEndPoint1 = endPoint - arrowDir * arrowHeadSize.x + orthogonal * arrowHeadSize.y;
		ImVec2 arrowEndPoint2 = endPoint - arrowDir * arrowHeadSize.x - orthogonal * arrowHeadSize.y;

		drawList->AddLine(startPoint, endPoint, color, lineWidth);
		drawList->AddLine(endPoint, arrowEndPoint1, color, lineWidth);
		drawList->AddLine(endPoint, arrowEndPoint2, color, lineWidth);
	}


	void AnimationGraphEditor::DrawNodes(PinPropertyContext& pinContext)
	{
		AnimationGraphNodeEditorModel* model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		m_State->HoveredNode = 0;
		m_State->HoveredTransition = 0;

		// If we've changed which node is the "root" of the rendered graph, then we need to re-load the graph settings.
		// Note that reloading settings only appears to work properly from _inside_ an ed::begin() / end() pair.
		if (ed::GetCurrentEditor() == m_SubGraph)
		{
			if (!model->IsSubGraphStateValid())
			{
				LoadSettings();
				model->InvalidateSubGraphState(false);
				// early exit and render (with reloaded settings) next frame
				return;
			}
		}
		else
		{
			if (!model->IsGraphStateValid())
			{
				LoadSettings();
				model->InvalidateGraphState(false);
				// early exit and render (with reloaded settings) next frame
				return;
			}
		}

		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsKeyDown(ImGuiKey_LeftAlt))
		{
			GetEditorContext()->EnableUserInput(false);
		}
		else
		{
			GetEditorContext()->EnableUserInput(true);
		}

		IONodeGraphEditor::DrawNodes(pinContext);

		// Editor can have either Nodes selected or Transitions, but not both.
		// This makes some tasks (e.g. deleting selected items) much easier.
		if ((ed::GetCurrentEditor() != m_SubGraph) && !GetSelectedNodeIDs().empty())
		{
			ClearSelectedTransitions();
		}

		if (!m_TransitionsToDelete.empty())
		{
			for (auto id : m_TransitionsToDelete)
			{
				GetModel()->RemoveNode(id);
			}
			ClearSelectedTransitions();
			m_TransitionsToDelete.clear();
		}

		if (m_State->TransitionStartNode != 0)
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || m_State->CreateNewNode || ImGui::IsPopupOpen("Create New Node"))
			{
				ImVec2 startPoint;
				ImVec2 endPoint;
				ImColor color;
				ImVec2 startNodePos = ed::GetNodePosition(ed::NodeId(m_State->TransitionStartNode));
				ImVec2 startNodeSize = ed::GetNodeSize(ed::NodeId(m_State->TransitionStartNode));

				if (m_State->HoveredNode == 0)
				{
					endPoint = m_State->CreateNewNode? m_State->LastTransitionEndPoint : ImGui::GetMousePos();
					startPoint = GetConnectionPointBetweenRectAndPoint({ startNodePos, startNodePos + startNodeSize }, endPoint);
					color = ed::GetStyle().Colors[ed::StyleColor_HovNodeBorder];
				}
				else
				{
					ImVec2 endNodePos = ed::GetNodePosition(ed::NodeId(m_State->HoveredNode));
					ImVec2 endNodeSize = ed::GetNodeSize(ed::NodeId(m_State->HoveredNode));
					GetConnectionPointsBetweenRects({ startNodePos, startNodePos + startNodeSize }, { endNodePos, endNodePos + endNodeSize }, startPoint, endPoint);
					color = ed::GetStyle().Colors[ed::StyleColor_SelNodeBorder];
				}

				DrawOffsetArrow(startPoint, endPoint, 5.0f, 3.0f, { 10.0f, 5.0f }, color);

				if (m_State->HoveredNode == 0 && !m_State->CreateNewNode)
				{
					auto* drawList = ImGui::GetWindowDrawList();

					auto labelPoint = endPoint + ImVec2(0.0f, -ImGui::GetTextLineHeight());
					auto size = ImGui::CalcTextSize("+ Create State");
					auto padding = ImGui::GetStyle().FramePadding;
					auto spacing = ImGui::GetStyle().ItemSpacing;

					labelPoint += ImVec2(spacing.x, -spacing.y);

					auto rectMin = labelPoint - padding;
					auto rectMax = labelPoint + size + padding;

					drawList->AddRectFilled(rectMin, rectMax, IM_COL32(32, 45, 32, 180), size.y * 0.15f);
					drawList->AddText(labelPoint, color, "+ Create State");
				}
			}
			else
			{
				if (m_State->HoveredNode == 0)
				{
					m_State->CreateNewNode = true;
					m_State->LastTransitionEndPoint = ImGui::GetMousePos();
					m_State->NewLinkPin = nullptr;

					ed::Suspend();
					ImGui::OpenPopup("Create New Node");
					m_State->NewNodePopupOpening = true;
					ed::Resume();
				}
				else
				{
					ed::ClearSelection();
					ClearSelectedTransitions();
					if (m_State->HoveredNode != m_State->TransitionStartNode)
					{
						for (auto& node : GetModel()->GetNodes())
						{
							if (node->GetTypeID() == AG::Types::ENodeType::Transition)
							{
								Node* startNode = GetModel()->GetNodeConnectedToPin(node->Inputs[0]->ID);
								Node* endNode = GetModel()->GetNodeConnectedToPin(node->Outputs[0]->ID);
								if (startNode && startNode->ID == m_State->TransitionStartNode && endNode && endNode->ID == m_State->HoveredNode)
								{
									SelectTransition(node->ID);
									break;
								}
							}
						}
						if (!IsAnyTransitionSelected())
						{
							Node* transitionNode = CreateTransition(m_State->TransitionStartNode, m_State->HoveredNode);
							SelectTransition(transitionNode->ID);

						}
					}
					m_State->TransitionStartNode = 0;
				}
			}
		}
	}


	void AnimationGraphEditor::DrawStateNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext)
	{
		builder.Middle();
		{
			ImGui::BeginHorizontal("##State");
			UI::ScopedColour headerTextColor(ImGuiCol_Text, IM_COL32(210, 210, 210, 255));

			static bool wasActive = false;

			static char buffer[256]{};
			memset(buffer, 0, sizeof(buffer));
			memcpy(buffer, node->Description.data(), std::min(node->Description.size(), sizeof(buffer)));

			const auto inputTextFlags = ImGuiInputTextFlags_AutoSelectAll;

			// Update our node size if backend node size has been changed
			const ImVec2 nodeSize = ed::GetNodeSize((uint64_t)node->ID);
			float availableWidth = 0.0f;
			if (nodeSize == ImVec2(0.0f, 0.0f))
			{
				availableWidth = node->Size.x;
			}
			else
			{
				availableWidth = nodeSize.x;
				if (nodeSize != node->Size)
					node->Size = nodeSize;
			}

			AG::Types::Node* stateNode = static_cast<AG::Types::Node*>(node);
			{
				UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
				UI::ScopedColour checkMark(ImGuiCol_CheckMark, IM_COL32(0, 210, 0, 255));
				bool entry = false;
				UI::ShiftCursor(-8.0, 2.0f);

				if (ImGui::RadioButton("##entry", stateNode->IsEntryState))
				{
					auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
					for (auto node : model->GetNodes())
					{
						static_cast<AG::Types::Node*>(node)->IsEntryState = false;
					}
					stateNode->IsEntryState = true;
				}
				UI::DrawItemActivityOutline(UI::OutlineFlags_NoOutlineInactive);
				UI::ShiftCursorY(-2.0f);
			}

			if (node->GetTypeID() == AG::Types::QuickState)
			{
				PinPropertyContext context{ node->Inputs[0], GetModel(), false, false };
				bool pinValueChanged = DrawPinPropertyEdit(context);

				if (context.OpenAssetPopup || context.OpenEnumPopup)
					pinContext = context;

				if (pinValueChanged && GetModel()->onPinValueChanged)
					GetModel()->onPinValueChanged(node->ID, node->Inputs[0]->ID);
			}
			else
			{
				ImVec2 textSize = ImGui::CalcTextSize((node->Description + "00").c_str());
				{
					ImGui::SetNextItemWidth(textSize.x);
					if (UI::InputText("##edit", buffer, 255, inputTextFlags))
					{
						node->Description = buffer;
					}
				}

				wasActive = DeactivateInputIfDragging(wasActive);
			}

			if (stateNode->IsEntryState)
			{
				ImGui::TextUnformatted("(entry)");
			}
			else
			{
				ImVec2 entryTextSize = ImGui::CalcTextSize("(entry)");
				ImGui::Dummy(entryTextSize);
			}
			ImGui::EndHorizontal();

			if (m_ShowNodeIDs)
				ImGui::TextUnformatted(("(" + std::to_string(node->ID) + ")").c_str());
			if (m_ShowSortIndices)
				ImGui::TextUnformatted(("(sort index: " + std::to_string(node->SortIndex) + ")").c_str());
		}
	}


	// TODO: This is not at all efficient.  Could be made better.
	// It will do for now
	float PointToOffsetArrowDistanceSquared(const ImVec2& point, ImVec2 startPoint, ImVec2 endPoint, const float offset)
	{
		ImVec2 arrowDir = ImVec2(endPoint - startPoint);
		float arrowLengthSquared = ImLengthSqr(arrowDir);
		float arrowLength = sqrt(arrowLengthSquared);
		if (arrowLength > 0.0)
		{
			arrowDir /= arrowLength;
		}
		ImVec2 orthogonal = { arrowDir.y, -arrowDir.x };
		ImVec2 offsetVec = (orthogonal * offset);

		startPoint += offsetVec;
		endPoint += offsetVec;

		ImVec2 pointToStart = point - startPoint;
		ImVec2 pointToEnd = point - endPoint;

		float dot1 = pointToStart.x * arrowDir.x + pointToStart.y * arrowDir.y;

		// return "infinity" if not directly to one side of the line or the other
		if (dot1 < 0)
			return FLT_MAX;
		else if ((dot1 * dot1) > arrowLengthSquared)
			return FLT_MAX;

		ImVec2 projection = startPoint + arrowDir * dot1;
		ImVec2 pointToProjection = point - projection;
		return (pointToProjection.x * pointToProjection.x) + (pointToProjection.y * pointToProjection.y);
	}


	void AnimationGraphEditor::DrawNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext)
	{
		ZONG_CORE_ASSERT(node->Type != NodeType::Comment);

		struct ScopedEdColor
		{
			ScopedEdColor(ax::NodeEditor::StyleColor index, ImVec4 color)
			{
				ed::PushStyleColor(index, color);
			}
			~ScopedEdColor()
			{
				ed::PopStyleColor();
			}
		};

		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());

		// Give the node a red border if it is not well defined
		// For instance, a state machine node is not well defined if it has no states.
		// A state node is not will defined it does not yet have a sub-graph.
		// A quick state node is not well defined if it does not have a valid animation asset assigned.
		ScopedEdColor nodeBorder(ax::NodeEditor::StyleColor_NodeBorder, model->IsWellDefined(node) ? ax::NodeEditor::GetStyle().Colors[ax::NodeEditor::StyleColor_NodeBorder] : ImVec4(0.9f, 0.2f, 0.2f, 1.0f));

		auto currentPath = model->GetCurrentPath();
		if (currentPath.empty() || (currentPath.back()->GetTypeID() != AG::Types::ENodeType::StateMachine))
		{
			return IONodeGraphEditor::DrawNode(node, builder, pinContext);
		}

		ZONG_CORE_ASSERT((node->GetTypeID() == AG::Types::ENodeType::State) || (node->GetTypeID() == AG::Types::QuickState) || (node->GetTypeID() == AG::Types::ENodeType::StateMachine) || (node->GetTypeID() == AG::Types::ENodeType::Transition));

		if ((node->GetTypeID() == AG::Types::ENodeType::State) || (node->GetTypeID() == AG::Types::QuickState) || (node->GetTypeID() == AG::Types::ENodeType::StateMachine))
		{
			ImVec4 color = node->Color;
			color.x /= 2.0f;
			color.y /= 2.0f;
			color.z /= 2.0f;
			ScopedEdColor nodeBg(ax::NodeEditor::StyleColor_NodeBg, color);

			builder.Begin(ed::NodeId(node->ID));

			DrawStateNode(node, builder, pinContext);

			builder.End();

			if(!ImGui::IsItemActive()) {
				// If we're holding down mouse button, ed::GetHoveredNode() doesn't return what we want.
				// Work out for ourselves if the mouse is over the node
				ImVec2 nodePos = ed::GetNodePosition(ed::NodeId(node->ID));
				ImVec2 nodeSize = ed::GetNodeSize(ed::NodeId(node->ID));
				ImRect rect(nodePos, nodePos + nodeSize);
				if (rect.Contains(ImGui::GetMousePos()))
				{
					m_State->HoveredNode = node->ID;

					if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsKeyDown(ImGuiKey_LeftAlt))
					{
						if (m_State->TransitionStartNode == 0)
						{
							m_State->TransitionStartNode = node->ID;
						}
					}

					if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && node->GetTypeID() != AG::Types::ENodeType::QuickState)
					{
						m_State->TransitionStartNode = 0;
						m_State->EditNodeId = node->ID;
					}
				}
			}
			int radius = EditorResources::ShadowTexture->GetHeight();// *1.4;
			UI::DrawShadow(EditorResources::ShadowTexture, radius);
		}
		else
		{
			// Transition Node
			// We custom draw these as arrows rather than as imgui_node_editor nodes
			Node* startNode = model->GetNodeConnectedToPin(node->Inputs[0]->ID);
			Node* endNode = model->GetNodeConnectedToPin(node->Outputs[0]->ID);
			ZONG_CORE_ASSERT(startNode && endNode);

			ImVec2 startPoint;
			ImVec2 endPoint;
			ImVec2 startNodePos = ed::GetNodePosition(ed::NodeId(startNode->ID));
			ImVec2 startNodeSize = ed::GetNodeSize(ed::NodeId(startNode->ID));
			ImVec2 endNodePos = ed::GetNodePosition(ed::NodeId(endNode->ID));
			ImVec2 endNodeSize = ed::GetNodeSize(ed::NodeId(endNode->ID));
			GetConnectionPointsBetweenRects({ startNodePos, startNodePos + startNodeSize }, { endNodePos, endNodePos + endNodeSize }, startPoint, endPoint);

			if (PointToOffsetArrowDistanceSquared(ImGui::GetMousePos(), startPoint, endPoint, 5.0f) < 25.0f)
			{
				m_State->HoveredTransition = node->ID;
				if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
				{
					ed::ClearSelection();
					SelectTransition(node->ID, ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl));
				}
			}

			if (IsTransitionSelected(node->ID))
			{
				ImColor color = ed::GetStyle().Colors[ed::StyleColor::StyleColor_SelLinkBorder];
				DrawOffsetArrow(startPoint, endPoint, 5.0f, 4.0f, { 10.0f, 5.0f }, color);
				if (ImGui::IsKeyPressed(ImGuiKey_Delete) && GetEditorContext()->GetViewRect().Contains(ImGui::GetMousePos()))
				{
					m_TransitionsToDelete.emplace(node->ID);
				}
			}
			else if (m_State->HoveredTransition == node->ID)
			{
				ImColor color = ed::GetStyle().Colors[ed::StyleColor::StyleColor_HovLinkBorder];
				DrawOffsetArrow(startPoint, endPoint, 5.0f, 3.0f, { 10.0f, 5.0f }, color);
			}
			
			// If transition graph is not defined, or always evaluates to false then draw transition arrow in red.
			// If transition graph always evaluates to true then draw transition arrow in green.
			// Otherwise, arrow is grey.
			ImU32 color = IM_COL32(192, 192, 192, 255);;
			auto transitionSubNodes = model->GetAllNodes().at(node->ID);
			if (transitionSubNodes.empty())
			{
				color = IM_COL32(230, 51, 51, 255);
			}
			else
			{
				auto it = std::find_if(transitionSubNodes.begin(), transitionSubNodes.end(), [endNode](const Node* n) { return n->Name == "Output"; });
				if (it == transitionSubNodes.end())
				{
					color = IM_COL32(230, 51, 51, 255);
				}
				else
				{
					auto outputNode = *it;
					if (outputNode->Inputs[0]->Value.isBool())
					{
						if (outputNode->Inputs[0]->Value.getBool())
						{
							color = IM_COL32(0, 210, 0, 255);
						}
						else
						{
							color = IM_COL32(230, 51, 51, 255);
						}
					}
					else
					{
						color = IM_COL32(192, 192, 192, 255);
					}
				}
			}
			DrawOffsetArrow(startPoint, endPoint, 5.0f, 2.0f, { 10.0f, 5.0f }, color);
		}
	}


	void AnimationGraphEditor::DrawGraphIO()
	{
		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		auto currentPath = model->GetCurrentPath();

		ImGui::Spacing();
		UI::BeginPropertyGrid();
		UI::PropertyAssetReferenceError error;
		AssetHandle skeleton = model->GetSkeletonHandle();
		UI::PropertyAssetReferenceSettings settings;
		if (!AssetManager::IsAssetHandleValid(skeleton))
		{
			settings.ButtonLabelColor = ImGui::ColorConvertU32ToFloat4(Colors::Theme::textError);
		}
		UI::PropertyAssetReference<SkeletonAsset>("Skeleton", skeleton, "",  & error, settings);
		if (error == UI::PropertyAssetReferenceError::None)
		{
			model->SetSkeletonHandle(skeleton);
		}
		UI::EndPropertyGrid();

		if (!currentPath.empty() && (currentPath.back()->GetTypeID() == AG::Types::ENodeType::StateMachine))
		{
			if ((m_SelectedTransitionNodes.size() == 1) && (GetEditorContext()->GetViewRect().Contains(ImGui::GetMousePos())))
			{
				auto currentTail = currentPath.back();
				auto selectedTransition = model->FindNode(*m_SelectedTransitionNodes.begin());
				ZONG_CORE_ASSERT(selectedTransition);
				model->SetCurrentPath(selectedTransition);
				IONodeGraphEditor::DrawGraphIO();
				model->SetCurrentPath(currentTail);
			}
			else
			{
				IONodeGraphEditor::DrawGraphIO();
			}
		}
		else
		{
			IONodeGraphEditor::DrawGraphIO();
		}
	}


	bool AnimationGraphEditor::DrawPinPropertyEdit(PinPropertyContext& context)
	{
		bool modified = false;

		Pin* pin = context.pin;

		switch ((AG::Types::EPinType)pin->GetType())
		{
			case AG::Types::EPinType::Bool:             modified = NodeEditorDraw::PropertyBool(pin->Value); break;
			case AG::Types::EPinType::Int:              modified = NodeEditorDraw::PropertyInt(pin->Value); break;
			case AG::Types::EPinType::Float:            modified = NodeEditorDraw::PropertyFloat(pin->Value); break;
			case AG::Types::EPinType::AnimationAsset:   modified = NodeEditorDraw::PropertyAsset<AnimationAsset, AssetType::Animation>(pin->Value, pin, context.OpenAssetPopup); break;
			case AG::Types::EPinType::String:           modified = NodeEditorDraw::PropertyString(pin->Value); break;
			case AG::Types::EPinType::SkeletonAsset:    modified = NodeEditorDraw::PropertyAsset<SkeletonAsset, AssetType::Skeleton>(pin->Value, pin, context.OpenAssetPopup); break;
			case AG::Types::EPinType::Enum:
			{
				const int enumValue = pin->Value["Value"].get<int>();

				// TODO: JP. not ideal to do this callback style value assignment, should make it more generic.
				auto constructEnumValue = [](int selected) { return ValueFrom(AnimationGraph::Types::TEnum{ selected }); };

				modified = NodeEditorDraw::PropertyEnum(enumValue, pin, context.OpenEnumPopup, constructEnumValue);
			}
			break;
			default:
				break;
		}

		return modified;
	}


	void AnimationGraphEditor::DrawPinIcon(const Pin* pin, bool connected, int alpha)
	{
		const int pinIconSize = GetPinIconSize();

		//IconType iconType;
		ImColor  color = GetIconColor(pin->GetType());
		color.Value.w = alpha / 255.0f;

		// Draw a highlight if hovering over this pin or its label
		if (ed::PinId(pin->ID) == ed::GetHoveredPin())
		{
			auto* drawList = ImGui::GetWindowDrawList();
			auto size = ImVec2(static_cast<float>(pinIconSize), static_cast<float>(pinIconSize));
			auto cursorPos = ImGui::GetCursorScreenPos();
			const auto outline_scale = size.x / 24.0f;
			const auto extra_segments = static_cast<int>(2 * outline_scale); // for full circle
			const auto radius = 0.5f * size.x / 2.0f - 0.5f;
			const auto centre = ImVec2(cursorPos.x + size.x / 2.0f, cursorPos.y + size.y / 2.0f);
			drawList->AddCircleFilled(centre, 0.5f * size.x, IM_COL32(255, 255, 255, 30), 12 + extra_segments);
		}

		// If this pin accepting a link, draw it as connected
		bool acceptingLink = IsPinAcceptingLink(pin);


		if (pin->Storage == StorageKind::Array)
			ax::Widgets::IconGrid(ImVec2(static_cast<float>(pinIconSize), static_cast<float>(pinIconSize)), connected || acceptingLink, color, ImColor(32, 32, 32, alpha));
		else
		{
			Ref<Texture2D> image;

			if (pin->IsType(AG::Types::EPinType::Flow))
			{
				image = connected || acceptingLink ? EditorResources::PinFlowConnectIcon : EditorResources::PinFlowDisconnectIcon;
			}
			else
			{
				image = connected || acceptingLink ? EditorResources::PinValueConnectIcon : EditorResources::PinValueDisconnectIcon;
			}

			const auto iconWidth = image->GetWidth();
			const auto iconHeight = image->GetHeight();
			ax::Widgets::ImageIcon(ImVec2(static_cast<float>(iconWidth), static_cast<float>(iconHeight)), UI::GetTextureID(image), connected, (float)pinIconSize, color, ImColor(32, 32, 32, alpha));
		}
	}

	Node* AnimationGraphEditor::CreateTransition(UUID startNodeID, UUID endNodeID)
	{
		if (startNodeID == endNodeID)
		{
			ZONG_CORE_ASSERT(false);
			return nullptr;
		}

		auto* model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		Node* transitionNode = model->CreateTransitionNode();
		if (transitionNode)
		{
			model->EnsureSubGraph(transitionNode);

			Node* startNode = GetModel()->FindNode(startNodeID);
			Node* endNode = GetModel()->FindNode(endNodeID);
			ZONG_CORE_ASSERT(startNode && endNode);

			Pin* pin = startNode->Outputs.emplace_back(AnimationGraph::Types::CreatePinForType(AG::Types::EPinType::Flow, "Transition"));
			pin->ID = UUID();
			pin->NodeID = startNode->ID;
			pin->Kind = PinKind::Output;

			pin = transitionNode->Inputs.emplace_back(AnimationGraph::Types::CreatePinForType(AG::Types::EPinType::Flow, "Transition"));
			pin->ID = UUID();
			pin->NodeID = transitionNode->ID;
			pin->Kind = PinKind::Input;

			pin = transitionNode->Outputs.emplace_back(AnimationGraph::Types::CreatePinForType(AG::Types::EPinType::Flow, "Transition"));
			pin->ID = UUID();
			pin->NodeID = transitionNode->ID;
			pin->Kind = PinKind::Output;

			pin = endNode->Inputs.emplace_back(AnimationGraph::Types::CreatePinForType(AG::Types::EPinType::Flow, "Transition"));
			pin->ID = UUID();
			pin->NodeID = endNode->ID;
			pin->Kind = PinKind::Input;

			GetModel()->CreateLink(startNode->Outputs.back(), transitionNode->Inputs.back());
			GetModel()->CreateLink(transitionNode->Outputs.back(), endNode->Inputs.back());
		}
		return transitionNode;
	}


	void AnimationGraphEditor::DeleteSelection()
	{
		// note: this will also delete links connected to the nodes
		auto selectedNodeIDs = GetSelectedNodeIDs();
		for (auto id : selectedNodeIDs)
			ed::DeleteNode(ed::NodeId(id));
	}


	void AnimationGraphEditor::CopySelection()
	{
		// We copy only the selected node ids, not the nodes themselves
		// This is simpler, but does mean that the nodes must still exist in order to paste them later.
		// i.e. a "cut" operation is not supported.
		m_CopiedNodeIDs = GetSelectedNodeIDs();

		// If copying from a state machine graph, then copy also any transition nodes where both ends of the transition is selected.
		auto* model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		if (!model->GetCurrentPath().empty() && model->GetCurrentPath().back()->GetTypeID() == AG::Types::ENodeType::StateMachine)
		{
			std::vector<UUID> copiedTransitionNodeIDs;
			for (auto id : m_CopiedNodeIDs)
			{
				auto* node = model->FindNode(id);
				if (!node)
					continue;

				for (auto* pin : node->Outputs)
				{
					if (pin->Name == "Transition")
					{
						auto* sourceLink = model->GetLinkConnectedToPin(pin->ID);
						if (sourceLink)
						{
							auto* endPin = model->FindPin(sourceLink->EndPinID);
							auto* transitionNode = endPin ? model->FindNode(endPin->NodeID) : nullptr;
							if (transitionNode)
							{
								auto* destLink = model->GetLinkConnectedToPin(transitionNode->Outputs[0]->ID);
								if (destLink)
								{
									auto* endPin = model->FindPin(destLink->EndPinID);
									auto* destNode = endPin ? model->FindNode(endPin->NodeID) : nullptr;
									if (destNode)
									{
										if (std::find(m_CopiedNodeIDs.begin(), m_CopiedNodeIDs.end(), destNode->ID) != m_CopiedNodeIDs.end())
										{
											copiedTransitionNodeIDs.push_back(transitionNode->ID);
										}
									}
								}
							}
						}
					}
				}
			}
			m_CopiedNodeIDs.insert(m_CopiedNodeIDs.end(), copiedTransitionNodeIDs.begin(), copiedTransitionNodeIDs.end());
		}
	}


	std::vector<UUID> AnimationGraphEditor::PasteSelection(const std::vector<UUID>& selectedNodeIDs, ImVec2 position)
	{
		std::vector<UUID> newNodeIDs;
		newNodeIDs.reserve(m_CopiedNodeIDs.size());

		if (selectedNodeIDs.empty())
			return newNodeIDs;

		auto* model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());

		// Early out if selected nodes are not right type for current graph.
		auto* selectedParent = model->FindParent(selectedNodeIDs.front());
		const auto& currentPath = model->GetCurrentPath();
		int selectedParentTypeID = selectedParent ? selectedParent->GetTypeID() : AG::Types::ENodeType::Animation;
		int currentPathTypeId = currentPath.empty() ? AG::Types::ENodeType::Animation : currentPath.back()->GetTypeID();
		if(selectedParentTypeID != currentPathTypeId)
			return newNodeIDs;

		// Early out if selected nodes are an ancestor of current graph
		// (pasting a parent into self is an infinite recursion)
		if (!currentPath.empty())
		{
			if(!selectedParent)
				return newNodeIDs;

			for (auto it = ++currentPath.rbegin(); it != currentPath.rend(); ++it)
			{
				if(*it == selectedParent)
					return newNodeIDs;
			}
		}

		std::unordered_map<UUID, UUID> oldToNewNodeIDMap;
		Node* firstNewNode = nullptr;
		ImVec2 nodePositionDelta;

		// For each node in copied node ids, create a copy of that node
		// and add it to the graph
		for (auto oldNodeID : selectedNodeIDs)
		{
			auto* oldNode = model->FindInAllNodes(oldNodeID);
			if (!oldNode)
				continue;

			Node* newNode = nullptr;
			if (model->IsPropertyNode(oldNode))
			{
				EPropertyType propertyType = model->GetPropertyTypeOfNode(oldNode);
				if (propertyType == EPropertyType::Input)
				{
					newNode = model->SpawnGraphInputNode(oldNode->Outputs[0]->Name);
				}
				else if (propertyType == EPropertyType::LocalVariable)
				{
					newNode = model->SpawnLocalVariableNode(oldNode->Name, !oldNode->Outputs.empty());
				}
				else
				{
					// we don't have graph output properties (yet)
					ZONG_CORE_ASSERT(false);
				}
			}
			else
			{
				if (oldNode->Category == "Utility")
				{
					newNode = model->CreateNode("Utility", "Comment");
					newNode->Name = oldNode->Name;
				}
				else if (oldNode->Category != "StateMachine" || oldNode->Name != "Transition")
				{
					newNode = model->CreateNode(oldNode->Category, oldNode->Name);
				}
			}

			if (!newNode)
				continue;

			if (!firstNewNode)
			{
				firstNewNode = newNode;
				nodePositionDelta = position - oldNode->GetPosition();
			}
			newNode->SetPosition(oldNode->GetPosition() + nodePositionDelta);
			newNode->Size = oldNode->Size;

			for (size_t i = 0; i < newNode->Inputs.size(); ++i)
			{
				newNode->Inputs[i]->Value = oldNode->Inputs[i]->Value;
			}

			newNode->Description = oldNode->Description;
			oldToNewNodeIDMap[oldNodeID] = newNode->ID;
			newNodeIDs.push_back(newNode->ID);
		}

		// Copy any links where both ends of the link have been copied
		std::vector<std::pair<Pin*, Pin*>> linksToCreate;
		auto& allLinks = model->GetAllLinks();
		for (const auto& [subgraph, links] : allLinks)
		{
			auto previousRoot = model->SetRootNodeID(subgraph);
			for (const auto& link : links)
			{
				auto* startNode = model->GetNodeForPin(link.StartPinID);
				auto* endNode = model->GetNodeForPin(link.EndPinID);

				auto startNodeID = startNode ? startNode->ID : UUID(0);
				auto endNodeID = endNode ? endNode->ID : UUID(0);

				if (oldToNewNodeIDMap.contains(startNodeID) && oldToNewNodeIDMap.contains(endNodeID))
				{
					// find the corresponding start and end pins on new nodes
					auto* newStartNode = model->FindInAllNodes(oldToNewNodeIDMap[startNodeID]);
					auto* newEndNode = model->FindInAllNodes(oldToNewNodeIDMap[endNodeID]);

					Pin* newStartPin = nullptr;
					Pin* newEndPin = nullptr;
					for (size_t i = 0; i < startNode->Outputs.size(); ++i)
					{
						if (startNode->Outputs[i]->ID == link.StartPinID)
						{
							newStartPin = newStartNode->Outputs[i];
							break;
						}
					}

					for (size_t i = 0; i < endNode->Inputs.size(); ++i)
					{
						if (endNode->Inputs[i]->ID == link.EndPinID)
						{
							newEndPin = newEndNode->Inputs[i];
							break;
						}
					}

					ZONG_CORE_ASSERT(newStartPin && newEndPin);

					linksToCreate.emplace_back(newStartPin, newEndPin);
				}
			}
			model->SetRootNodeID(previousRoot);
		}

		for (auto [startPin, endPin] : linksToCreate)
		{
			model->CreateLink(startPin, endPin);
		}

		// Copy state machine transition nodes (if any)
		for (auto oldNodeID : selectedNodeIDs)
		{
			auto* oldNode = model->FindInAllNodes(oldNodeID);
			if (!oldNode)
				continue;

			if (oldNode->Category != "StateMachine" || oldNode->Name != "Transition")
				continue;

			auto* previousRoot = model->SetCurrentPath(model->FindParent(oldNodeID));
			auto startNode = model->GetNodeConnectedToPin(oldNode->Inputs[0]->ID);
			auto endNode = model->GetNodeConnectedToPin(oldNode->Outputs[0]->ID);
			model->SetCurrentPath(previousRoot);

			if (startNode && endNode)
			{
				auto* newNode = CreateTransition(oldToNewNodeIDMap[startNode->ID], oldToNewNodeIDMap[endNode->ID]);
				oldToNewNodeIDMap[oldNodeID] = newNode->ID;
			}
		}

		// Deep copy sub-graphs, if any
		auto& allNodes = model->GetAllNodes();
		auto& allGraphStates = model->GetAllGraphStates();
		for (auto oldNodeID : selectedNodeIDs)
		{
			if (!allNodes.contains(oldNodeID))
				continue; // not a sub-graph

			auto newNodeID = oldToNewNodeIDMap[oldNodeID];
			Node* newNode = model->FindInAllNodes(newNodeID);
			ZONG_CORE_ASSERT(newNode);

			auto* previousRootNode = model->SetCurrentPath(newNode);

			allNodes[newNodeID] = {};
			allLinks[newNodeID] = {};
			allGraphStates[newNodeID] = {};

			std::vector<UUID> subgraphNodeIDs;
			subgraphNodeIDs.reserve(allNodes[oldNodeID].size());
			for (const auto& node : allNodes[oldNodeID])
			{
				subgraphNodeIDs.push_back(node->ID);
			}

			PasteSelection(subgraphNodeIDs, allNodes[oldNodeID].front()->GetPosition());
			model->SetCurrentPath(previousRootNode);
		}

		return newNodeIDs;
	}


	void AnimationGraphEditor::DrawNodeContextMenu(Node* node)
	{
		auto selectedNodes = GetSelectedNodeIDs();

		if (selectedNodes.empty())
			return;

		if (ImGui::MenuItem("Delete"))
			DeleteSelection();

		ImGui::Separator();

		// Cut is not supported, since we are doing a shallow copy of the nodes only
		// (so if you "cut" them, they will no longer exist and so cannot be pasted)
		//
		// How could we support a proper cut and copy?
		// We'd need to have another AnimationGraph and another model referencing that graph.
		// Then we'd need to "cut" / "copy" the selected nodes/links into that graph.
		// Then copy them back again on "paste"
		//if (ImGui::MenuItem("Cut"))
		//{
		//	CopySelection();
		//	DeleteSelection();
		//}

		if (ImGui::MenuItem("Copy"))
			CopySelection();

		if (ImGui::MenuItem("Paste"))
		{
			ed::ClearSelection();
			auto newNodeIDs = PasteSelection(m_CopiedNodeIDs, ed::ScreenToCanvas(ImGui::GetMousePos()));
			for (auto newNodeID : newNodeIDs)
			{
				ed::SelectNode(ed::NodeId(newNodeID));
			}
		}
	}

}
