#include "hzpch.h"
#include "AnimationGraphEditor.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Editor/NodeGraphEditor/NodeGraphEditorContext.h"
#include "Hazel/Vendor/imgui-node-editor/widgets.h"

#include <CDT/CDT.h>
#include <choc/text/choc_StringUtilities.h>
#include <imgui-node-editor/builders.h>
#include <imgui-node-editor/imgui_node_editor_internal.h>

#include <format>

namespace ed = ax::NodeEditor;
namespace AG = Hazel::AnimationGraph;

static ax::NodeEditor::Detail::EditorContext* GetEditorContext()
{
	return (ax::NodeEditor::Detail::EditorContext*)(ed::GetCurrentEditor());
}

namespace Hazel {

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


	void DrawVertex(ImVec2 point, const float radius, ImU32 color)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddCircleFilled(point, radius, color);
	}


	enum class ELocation
	{
		Above,
		Below
	};


	void DrawVertexCoords(ImVec2 blendSpacePos, ImVec2 nodePos, ELocation where)
	{
		auto* drawList = ImGui::GetWindowDrawList();
		std::string coords = std::format("({:.2f}, {:.2f})", blendSpacePos.x, blendSpacePos.y);
		ImVec2 coordsSize = ImGui::CalcTextSize(coords.c_str());
		auto coordsPos = nodePos + ImVec2{ -coordsSize.x / 2.0f, 0.0f };
		coordsPos.y += (coordsSize.y + GImGui->Style.FramePadding.y) * (where == ELocation::Above? -1.0f : 1.0f);
		drawList->AddText(coordsPos, Colors::Theme::textDarker, coords.c_str());
	}


	struct BonePinContext
	{
		UUID PinID = 0;
		std::string_view SelectedBone;

		std::vector<Token> BoneTokens;
	};

	static BonePinContext s_SelectedBoneContext;


	AnimationGraphEditor::AnimationGraphEditor() : IONodeGraphEditor("Animation Graph Editor")
	{
		m_NodeBuilderTexture = EditorResources::TranslucentTexture;

		onDragDropOntoCanvas = [&]() {
			// Only state machine or blendspace sub-graphs accept a dropped asset
			// Only graphs other than state machine or blendspace accept a dropped graph input
			auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
			auto currentPath = model->GetCurrentPath();
			bool isStateMachine = !currentPath.empty() && (currentPath.back()->GetTypeID() == AG::Types::ENodeType::StateMachine);
			bool isBlendSpace = !currentPath.empty() && (currentPath.back()->GetTypeID() == AG::Types::ENodeType::BlendSpace);
			if (isStateMachine || isBlendSpace)
			{
				if (auto data = ImGui::AcceptDragDropPayload("asset_payload", ImGuiDragDropFlags_AcceptBeforeDelivery); data != nullptr)
				{
					AssetHandle assetHandle = *(AssetHandle*)data->Data;
					Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
					if (asset && asset->GetAssetType() == AssetType::Animation)
					{
						auto getBlendSpacePos = [&](ImVec2 mousePos) {
							auto blendSpace = static_cast<AG::Types::Node*>(currentPath.back());
							ImVec2 nodePos = ed::ScreenToCanvas(mousePos);
							return blendSpace->Offset + blendSpace->Scale * nodePos;
						};

						if (isBlendSpace && data->IsPreview())
						{
							ImVec2 mousePos = ImGui::GetMousePos();
							DrawVertex(mousePos, s_BlendSpaceVertexRadius, ImGui::GetColorU32(ImGuiCol_DragDropTarget));
							DrawVertexCoords(getBlendSpacePos(mousePos), mousePos, ELocation::Above);
						}

						if (data->IsDelivery())
						{
							Hazel::Node* node = nullptr;
							if (isStateMachine)
							{
								node = model->CreateNode("StateMachine", "Quick State");
							}
							else if (isBlendSpace)
							{
								node = model->CreateNode("BlendSpace", "Blend Space Vertex");
								auto blendSpaceVertex = static_cast<AG::Types::Node*>(node);
								ImVec2 mousePos = ImGui::GetMousePos();
								blendSpaceVertex->Offset = getBlendSpacePos(mousePos);
							}
							HZ_CORE_ASSERT(node);
							ed::SetNodePosition(ed::NodeId(node->ID), ed::ScreenToCanvas(ImGui::GetMousePos()));
							node->Inputs[0]->Value = Utils::CreateAssetHandleValue<AssetType::Animation>(assetHandle);
							if (GetModel()->onPinValueChanged)
								GetModel()->onPinValueChanged(node->ID, node->Inputs[0]->ID);
						}
					}
				}
			}
			else
			{
				Hazel::Node* node = nullptr;
				if (auto data = ImGui::AcceptDragDropPayload("input_payload"); data != nullptr)
				{
					node = model->SpawnGraphInputNode((char*)data->Data);
				}
				if (auto data = ImGui::AcceptDragDropPayload("localvar_payload"); data != nullptr)
				{
					node = model->SpawnLocalVariableNode((char*)data->Data, !ImGui::IsKeyDown(ImGuiKey_LeftShift) && !ImGui::IsKeyDown(ImGuiKey_RightShift));
				}
				if (node)
				{
					// it can legitimately happen that the node is not created (e.g. user drops the payload into a completely different graph)
					ed::SetNodePosition(ed::NodeId(node->ID), ed::ScreenToCanvas(ImGui::GetMousePos()));
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
		const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(asset->Handle);
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


	void AnimationGraphEditor::OnRenderOnCanvas(ImVec2 min, ImVec2 max)
	{
		static auto textSizeX = ImGui::CalcTextSize("X");
		static auto textSizeY = ImGui::CalcTextSize("Y");
		static auto textSizeLerpSeconds = ImGui::CalcTextSize("Lerp seconds");
		static auto textSizePerUnitY = ImGui::CalcTextSize("per unit Y");
		static auto textSizeLerpSecondsPerUnitX = ImGui::CalcTextSize("Lerp seconds per unit X");
		static auto textSizeShowVertexDetails = ImGui::CalcTextSize("Show Vertex Details");

		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		float itemWidth = 100.0f;
		float spacing = 5.0f /* * scaleFactor*/;
		float margin = 10.0f /* * scaleFactor*/;
		{
			UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
			const ImVec2 linkPos({ margin, margin });
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
		}

		if (!model->GetCurrentPath().empty())
		{
			auto typeID = model->GetCurrentPath().back()->GetTypeID();
			if (typeID == AG::Types::ENodeType::StateMachine)
			{
				UI::ScopedFont normalFont(ImGui::GetIO().Fonts->Fonts[0]);
				UI::ScopedColour darkText(ImGuiCol_Text, Colors::Theme::textDarker);
				UI::ShiftCursor(20.0f, 20.0f);
				ImGui::TextUnformatted("Double click to edit a state.");
				UI::ShiftCursorX(20.0f);
				ImGui::TextUnformatted("ALT + click and drag to create transitions.");
			}
			else if (typeID == AG::Types::ENodeType::BlendSpace)
			{
				auto blendSpace = static_cast<AG::Types::Node*>(model->GetCurrentPath().back());
				UI::ScopedFont normalFont(ImGui::GetIO().Fonts->Fonts[0]);
				UI::ScopedColour darkText(ImGuiCol_Text, Colors::Theme::textDarker);
				UI::ShiftCursor(margin, margin);
				ImGui::TextUnformatted("Right click to create a vertex, or drag'n'drop an animation asset into the blend space.");
				UI::ShiftCursorX(margin);
				ImGui::TextUnformatted("Link icon next to a vertex indicates that animation playback will be synchronized.");
				ImGui::SameLine();
				auto detailsSize = ImGui::CalcTextSize("Show Vertex Details").x;
				UI::ShiftCursorX((max.x - min.x) - ImGui::GetCursorPosX() - detailsSize - ImGui::GetFrameHeight() - 2.0f * ImGui::GetStyle().FramePadding.x - margin);
				UI::Checkbox("##ShowVertexDetails", &blendSpace->ShowDetails);
				ImGui::SameLine();
				ImGui::TextUnformatted("Show Vertex Details");

				auto drawDashedLine = [](ImVec2 pos1, ImVec2 pos2)
				{
					const ImVec2 lineDir = pos2 - pos1;
					const float lineLength = ImLength(lineDir);
					const ImVec2 lineDirNorm = lineDir / lineLength;
					const float dashLength = 5.0f /* * scaleFactor */;
					const float gapLength = 5.0f /* * scaleFactor */;
					const float numDashes = lineLength / (dashLength + gapLength);
					const float dashGapLength = gapLength + dashLength;
					const ImVec2 dashGapDir = lineDirNorm * dashGapLength;
					const ImVec2 dashDir = lineDirNorm * dashLength;
					ImVec2 dashStart = pos1;
					for (int i = 0; i < numDashes; i++)
					{
						ImGui::GetWindowDrawList()->AddLine(dashStart, dashStart + dashDir * std::min(1.0f, numDashes - i), Colors::Theme::textDarker);
						dashStart += dashGapDir;
					}
				};

				auto recalculateBlendSpaceVertices = [&]()
				{
					for (auto node : model->GetNodes())
					{
						auto nodePos = static_cast<AG::Types::Node*>(node)->Offset;
						ed::SetNodePosition(ed::NodeId(node->ID), (nodePos - blendSpace->Offset) / blendSpace->Scale);
					}
				};

				auto pos1 = min + ImGui::GetCursorPos() + ImVec2{ margin + spacing + itemWidth, 0.0f };
				auto pos2 = ImVec2{ pos1.x, max.y - margin - (ImGui::GetFrameHeight() + spacing) * 2.0f};
				auto pos3 = ImVec2{ max.x - margin, pos2.y };
				auto pos4 = ImVec2{ pos3.x, pos1.y };
				drawDashedLine(pos1, pos2);
				drawDashedLine(pos2, pos3);
				drawDashedLine(pos3, pos4);
				drawDashedLine(pos4, pos1);

				auto pos5 = ImVec2{ min.x + margin + itemWidth / 2.0f, pos1.y + ImGui::GetFrameHeight() + spacing };
				auto pos6 = ImVec2{ pos5.x, (pos1.y + pos2.y - ImGui::GetTextLineHeight() * 4 - spacing * 5) / 2.0f };
				auto pos7 = ImVec2{ pos5.x, pos6.y + ImGui::GetTextLineHeight() * 4 + spacing * 5 };
				auto pos8 = ImVec2{ pos5.x, pos2.y - ImGui::GetFrameHeight() - 2.0f * spacing};
				DrawOffsetArrow(pos6, pos5, 0.0f, 1.0f, {2.0f * spacing, spacing }, Colors::Theme::textDarker);
				DrawOffsetArrow(pos7, pos8, 0.0f, 1.0f, {2.0f * spacing, spacing }, Colors::Theme::textDarker);

				auto pos9 = ImVec2{ pos2.x + itemWidth + spacing, pos2.y + spacing + ImGui::GetFrameHeight() / 2.0f };
				auto pos10 = ImVec2{ min.x + (max.x - min.x + itemWidth) / 2.0f - spacing, pos9.y };
				auto pos11 = ImVec2{ pos10.x + textSizeX.x + 2 * spacing, pos9.y };
				auto pos12 = ImVec2{ pos3.x - itemWidth - spacing, pos9.y };
				DrawOffsetArrow(pos10, pos9, 0.0f, 1.0f, { 2.0f * spacing, spacing }, Colors::Theme::textDarker);
				DrawOffsetArrow(pos11, pos12, 0.0f, 1.0f, { 2.0f * spacing, spacing }, Colors::Theme::textDarker);

				ImVec2 canvasMin = ed::ScreenToCanvas(pos1);
				ImVec2 canvasMax = ed::ScreenToCanvas(pos3);
				ImVec2 blendSpaceMin = blendSpace->Offset + blendSpace->Scale * canvasMin;
				ImVec2 blendSpaceMax = blendSpace->Offset + blendSpace->Scale * canvasMax;

				ImGui::SetNextItemWidth(itemWidth);
				UI::ShiftCursorX(margin);
				if(UI::DragFloat("##minY", &blendSpaceMin.y, 1.0f, -FLT_MAX, FLT_MAX, "%.2f")) {
					blendSpace->Scale.y = (blendSpaceMax.y - blendSpaceMin.y) / (canvasMax.y - canvasMin.y);
					blendSpace->Offset.y = -blendSpace->Scale.y * canvasMin.y + blendSpaceMin.y;
					recalculateBlendSpaceVertices();
				}

				ImGui::SetCursorPos({ margin + (itemWidth - textSizeY.x) / 2.0f, pos6.y + spacing - min.y });
				ImGui::TextUnformatted("Y");
				ImGui::SetCursorPosX(margin/2.0f + (itemWidth - textSizeLerpSeconds.x) / 2.0f);
				ImGui::TextUnformatted("Lerp seconds");
				ImGui::SetCursorPosX(margin/2.0f + (itemWidth - textSizePerUnitY.x) / 2.0f);
				ImGui::TextUnformatted("per unit Y");
				UI::ShiftCursorX(margin);
				ImGui::SetNextItemWidth(itemWidth);
				UI::DragFloat("##LerpSecondsY", &blendSpace->LerpSecondsPerUnit.y, 1.0f, 0.0f, FLT_MAX, "%0.2f");

				ImGui::SetCursorPos({ margin, pos2.y - ImGui::GetFrameHeight() - min.y });
				ImGui::SetNextItemWidth(itemWidth);
				if(UI::DragFloat("##maxY", &blendSpaceMax.y, 1.0f, -FLT_MAX, FLT_MAX, "%.2f")) {
					blendSpace->Scale.y = (blendSpaceMax.y - blendSpaceMin.y) / (canvasMax.y - canvasMin.y);
					blendSpace->Offset.y = -blendSpace->Scale.y * canvasMin.y + blendSpaceMin.y;
					recalculateBlendSpaceVertices();
				}

				ImGui::SetCursorPos({ margin + itemWidth + spacing, pos2.y + 2.0f * spacing - min.y });
				ImGui::SetNextItemWidth(itemWidth);
				if (UI::DragFloat("##minX", &blendSpaceMin.x, 1.0f, -FLT_MAX, FLT_MAX, "%.2f"))
				{
					blendSpace->Scale.x = (blendSpaceMax.x - blendSpaceMin.x) / (canvasMax.x - canvasMin.x);
					blendSpace->Offset.x = -blendSpace->Scale.x * canvasMin.x + blendSpaceMin.x;
					recalculateBlendSpaceVertices();
				}
				ImGui::SetCursorPos({ (max.x - min.x + itemWidth) / 2.0f, pos2.y + 2.0f * spacing + ImGui::GetStyle().FramePadding.y - min.y});
				ImGui::TextUnformatted("X");
				ImGui::SetCursorPosX((max.x - min.x - textSizeLerpSecondsPerUnitX.x) / 2.0f);
				ImGui::TextUnformatted("Lerp seconds per unit X");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(itemWidth);
				UI::DragFloat("##LerpSecondsX", &blendSpace->LerpSecondsPerUnit.x, 1.0f, 0.0f, FLT_MAX, "%0.2f");

				ImGui::SetCursorPos({max.x - min.x - margin - itemWidth, pos2.y + 2.0f * spacing - min.y });
				ImGui::SetNextItemWidth(itemWidth);
				if (UI::DragFloat("##maxX", &blendSpaceMax.x, 1.0f, -FLT_MAX, FLT_MAX, "%.2f"))
				{
					blendSpace->Scale.x = (blendSpaceMax.x - blendSpaceMin.x) / (canvasMax.x - canvasMin.x);
					blendSpace->Offset.x = -blendSpace->Scale.x * canvasMin.x + blendSpaceMin.x;
					recalculateBlendSpaceVertices();
				}
			}
		}
	}


	void AnimationGraphEditor::OnRender()
	{
		IONodeGraphEditor::OnRender();

		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		auto currentPath = model->GetCurrentPath();
		if (!currentPath.empty())
		{
			if (currentPath.back()->GetTypeID() == AG::Types::ENodeType::StateMachine)
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
							HZ_CORE_ASSERT(selectedTransition);
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
							HZ_CORE_ASSERT(startNode && endNode);
							{
								UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
								UI::ScopedColour darkText(ImGuiCol_Text, Colors::Theme::textDarker);
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
								UI::ScopedColour darkText(ImGuiCol_Text, Colors::Theme::textDarker);
								ImGui::SetCursorPos({ 20.0f, 20.0f });
								ImGui::TextUnformatted("No transition (or multiple) selected");
							}
						}
						ed::SetCurrentEditor(m_Editor);
					}
				}
				ImGui::End(); // Subgraph
			}
			else if (currentPath.back()->GetTypeID() == AG::Types::ENodeType::BlendSpace)
			{
				auto* editor = GetEditorContext();
				if (auto* action = editor->GetCurrentAction())
				{
					if (auto* selectAction = action->AsSelect())
					{
						if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_RightCtrl))
						{
							ClearSelectedBlendSpaceVertices();
						}
						ImRect rect = ImRect(selectAction->m_StartPoint, selectAction->m_EndPoint);
						for (auto blendSpaceVertex : model->GetAllNodes()[currentPath.back()->ID])
						{
							if (rect.Contains(blendSpaceVertex->GetPosition()))
							{
								SelectBlendSpaceVertex(blendSpaceVertex->ID, true);
							}
						}
					}
				}
				else
				{
					// Deselect vertex if user clicks somewhere else.
					// Currently commented out because it interferes with situations where the user
					// definitely does NOT want the vertex to deselect (e.g. click on animation asset chooser, or linl/unlink button)
					//if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && !m_State->HoveredVertex)
					//{
					//	ClearSelectedBlendSpaceVertices();
					//}
				}
			}
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

		HZ_CORE_ASSERT(ed::GetCurrentEditor() == m_Editor);

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


	void AnimationGraphEditor::DrawNodes(PinPropertyContext& pinContext)
	{
		AnimationGraphNodeEditorModel* model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		m_State->HoveredNode = 0;
		m_State->HoveredTransition = 0;
		m_State->HoveredVertex = 0;

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

		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsKeyDown(ImGuiKey_LeftAlt) || (m_State->DraggingVertex != 0))
		{
			GetEditorContext()->EnableUserInput(false);
		}
		else
		{
			GetEditorContext()->EnableUserInput(true);
		}

		auto currentPath = model->GetCurrentPath();
		bool blendSpace = false;
		if (!currentPath.empty() && (currentPath.back()->GetTypeID() == AG::Types::ENodeType::BlendSpace))
		{
			blendSpace = true;
		}
		if(blendSpace) {
			ImGui::GetWindowDrawList()->ChannelsSetCurrent(ImGui::GetWindowDrawList()->_Splitter._Count - 1); //! (0x) not ideal using internal _Splitter._Count, but I have not figured out how to do it properly.
		}
		IONodeGraphEditor::DrawNodes(pinContext);
		if (blendSpace)
		{
			ImGui::GetWindowDrawList()->ChannelsSetCurrent(ImGui::GetWindowDrawList()->_Splitter._Count - 2); //! (0x) not ideal using internal _Splitter._Count, but I have not figured out how to do it properly.
			DrawBlendSpaceTriangles();
			ImGui::GetWindowDrawList()->ChannelsSetCurrent(0);
		}

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

		if (blendSpace)
		{
			if (!m_BlendSpaceVerticesToDelete.empty())
			{
				for (auto id : m_BlendSpaceVerticesToDelete)
				{
					GetModel()->RemoveNode(id);
				}
				ClearSelectedBlendSpaceVertices();
				m_BlendSpaceVerticesToDelete.clear();
			}

			if (m_State->DraggingVertex != 0)
			{
				// only set position if the mouse is actually dragging
				// (otherwise the action of selecting a vertex can cause it to move slightly)
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
				{
					auto blendSpace = static_cast<AG::Types::Node*>(model->GetCurrentPath().back());
					auto blendSpaceVertex = static_cast<AG::Types::Node*>(GetModel()->FindNode(m_State->DraggingVertex));
					ImVec2 snappedMousePos = ImGui::GetMousePos();
					ImVec2 blendSpaceMousePos = blendSpace->Offset + blendSpace->Scale * snappedMousePos;

					// snap blend space coordinates, unless ctrl is held
					if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_RightCtrl))
					{
						// work out how many decimal places to snap to (basically which power of 10 corresponds to 10 pixels)
						ImVec2 blendSpaceMousePosPlus1 = blendSpaceMousePos + ImVec2{ 1.0f, 1.0f };
						ImVec2 snappedMousePosPlus1 = (blendSpaceMousePosPlus1 - blendSpace->Offset) / blendSpace->Scale;
						ImVec2 screenPos = ed::CanvasToScreen(snappedMousePos);
						ImVec2 screenPosPlus1 = ed::CanvasToScreen(snappedMousePosPlus1);
						float dp = glm::floor((glm::log(ImLength(screenPosPlus1 - screenPos)) / glm::log(10.0f))) - 1.0f;
						float scaling = glm::pow(10.0f, dp);

						blendSpaceMousePos = (ImFloor(blendSpaceMousePos * scaling + ImVec2(0.5f, 0.5f))) / scaling;
						snappedMousePos = (blendSpaceMousePos - blendSpace->Offset) / blendSpace->Scale;
					}
					// We have to store blend space vertex position additionally to editor node position
					// because editor node position is stored as integer (and so loses precision)
					blendSpaceVertex->Offset = blendSpaceMousePos;
					ed::SetNodePosition(ed::NodeId(m_State->DraggingVertex), snappedMousePos);
					m_State->DraggedVertex = m_State->DraggingVertex;
				}
			}
		}
	}


	void AnimationGraphEditor::DrawNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext)
	{
		HZ_CORE_ASSERT(node->Type != NodeType::Comment);
		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());

		// Give the node a red border if it is not well defined
		// For instance, a state machine node is not well defined if it has no states.
		// A state node is not will defined it does not yet have a sub-graph.
		// A quick state node is not well defined if it does not have a valid animation asset assigned.
		ScopedEdColor nodeBorder(ax::NodeEditor::StyleColor_NodeBorder, model->GetNodeError(node).empty() ? ax::NodeEditor::GetStyle().Colors[ax::NodeEditor::StyleColor_NodeBorder] : ImVec4(0.9f, 0.2f, 0.2f, 1.0f));

		auto currentPath = model->GetCurrentPath();
		if (!currentPath.empty() && (currentPath.back()->GetTypeID() == AG::Types::ENodeType::StateMachine))
		{
			DrawNodeForStateMachine(node, builder, pinContext);
			return;
		}
		if (!currentPath.empty() && (currentPath.back()->GetTypeID() == AG::Types::ENodeType::BlendSpace))
		{
			DrawNodeForBlendSpace(node, builder, pinContext);
			return;
		}
		IONodeGraphEditor::DrawNode(node, builder, pinContext);
	}


	// TODO: This is not at all efficient.  Could be made better.
	// It will do for now
	float PointToOffsetArrowDistanceSquared(const ImVec2& point, ImVec2 startPoint, ImVec2 endPoint, const float offset)
	{
		ImVec2 arrowDir = endPoint - startPoint;
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


	void AnimationGraphEditor::DrawNodeForStateMachine(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext)
	{
		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());

		HZ_CORE_ASSERT((node->GetTypeID() == AG::Types::ENodeType::State) || (node->GetTypeID() == AG::Types::QuickState) || (node->GetTypeID() == AG::Types::ENodeType::StateMachine) || (node->GetTypeID() == AG::Types::ENodeType::Transition));
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

			if (!ImGui::IsItemActive())
			{
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
			HZ_CORE_ASSERT(startNode && endNode);

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

			// If transition graph is ill defined then draw transition arrow in red.
			// Otherwise, arrow is grey.
			ImU32 color = IM_COL32(192, 192, 192, 255);;
			auto transitionSubNodes = model->GetAllNodes().at(node->ID);
			if (transitionSubNodes.empty() || !model->GetNodeError(node).empty())
			{
				color = IM_COL32(230, 51, 51, 255);
			}
			DrawOffsetArrow(startPoint, endPoint, 5.0f, 2.0f, { 10.0f, 5.0f }, color);
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


	float PointToVertexDistanceSquared(const ImVec2& point, ImVec2 vertex)
	{
		ImVec2 diff = point - vertex;
		return ImLengthSqr(diff);
	}


	void AnimationGraphEditor::DrawNodeForBlendSpace(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext)
	{
		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		auto blendSpace = static_cast<AG::Types::Node*>(model->GetCurrentPath().back());
		auto blendSpaceVertex = static_cast<AG::Types::Node*>(node);

		//HZ_CORE_ASSERT(node->GetTypeID() == AG::Types::ENodeType::BlendSpaceVertex);

		ImVec2 nodePos = ed::GetNodePosition(ed::NodeId(node->ID));

		auto drawVertexLabels = [&] {
			UI::ScopedID id(node->ID);
			ImGui::SetCursorPos(nodePos + ImVec2(10.0f, -(ImGui::GetTextLineHeight() / 2.0f + GImGui->Style.FramePadding.y)));
			UI::ScopedFont fontNormal(ImGui::GetIO().Fonts->Fonts[2]);
			pinContext.pin = node->Inputs[0];
			DrawPinPropertyEdit(pinContext);
			ImGui::SameLine();
			auto icon = node->Inputs[1]->Value.getBool()? EditorResources::LinkedIcon : EditorResources::UnlinkedIcon;
			const float height = std::min((float)icon->GetHeight(), 18.0f);
			const float width = (float)icon->GetWidth() / (float)icon->GetHeight() * height;
			const bool clicked = ImGui::InvisibleButton(UI::GenerateID(), ImVec2(width, height));
			UI::DrawButtonImage(icon, Colors::Theme::text, Colors::Theme::textBrighter, Colors::Theme::accent, UI::GetItemRect());
			if (clicked)
			{
				node->Inputs[1]->Value = choc::value::createBool(!node->Inputs[1]->Value.getBool());
			}
			//UI::ScopedColour darkText(ImGuiCol_Text, Colors::Theme::textDarker);
			//UI::ScopedFont smallFont(ImGui::GetIO().Fonts->Fonts[5]); //? not ideal assuming 5 is always going to be small font
			DrawVertexCoords(blendSpaceVertex->Offset, nodePos, ELocation::Below);
		};

		if ((PointToVertexDistanceSquared(ImGui::GetMousePos(), nodePos) < 25.0f) || (m_State->DraggingVertex == node->ID))
		{
			m_State->HoveredVertex = node->ID;
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !(ImGui::IsMouseDragging(ImGuiMouseButton_Left) && m_State->DraggingVertex == 0))
			{
				m_State->DraggingVertex = node->ID;
			}
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				if (m_State->DraggedVertex != 0)
				{
					m_State->DraggingVertex = 0;
					m_State->DraggedVertex = 0;
				}
				else
				{
					ed::ClearSelection();
					m_State->DraggingVertex = 0;
					SelectBlendSpaceVertex(node->ID, ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl));
				}
			}
			drawVertexLabels();
		}

		if (IsBlendSpaceVertexSelected(node->ID))
		{
			ImColor color = ed::GetStyle().Colors[ed::StyleColor::StyleColor_SelLinkBorder];
			DrawVertex(nodePos, s_BlendSpaceVertexRadius + 1.0f, color);
			if (ImGui::IsKeyPressed(ImGuiKey_Delete) && GetEditorContext()->GetViewRect().Contains(ImGui::GetMousePos()))
			{
				m_BlendSpaceVerticesToDelete.emplace(node->ID);
			}
			drawVertexLabels();
		}
		else if (m_State->HoveredVertex == node->ID)
		{
			ImColor color = ed::GetStyle().Colors[ed::StyleColor::StyleColor_HovLinkBorder];
			DrawVertex(nodePos, s_BlendSpaceVertexRadius + 1.0f, color);
		}

		ImU32 color = AG::Types::GetPinColor(AG::Types::EPinType::AnimationAsset);
		if(!model->GetNodeError(node).empty())
		{
			color = IM_COL32(230, 51, 51, 255);
		}
		DrawVertex(nodePos, s_BlendSpaceVertexRadius, color);
		if (blendSpace->ShowDetails)
		{
			drawVertexLabels();
		}
	}


	void AnimationGraphEditor::DrawBlendSpaceTriangles()
	{
		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		auto blendSpace = static_cast<AG::Types::Node*>(model->GetCurrentPath().back());
		auto nodes = model->GetNodes();

		// Make sure to do the triangulation in blend space coordinates, since that is what the final compiled node is going to do.
		// Also, need to remove duplicates (which is a pity as otherwise we could have used CDT's custom vertex API to access nodes directly)
		std::vector<CDT::V2d<float>> vertices;
		vertices.reserve(nodes.size());
		for (auto node : nodes)
		{
			auto blendSpaceVertex = static_cast<AG::Types::Node*>(node);
			vertices.emplace_back(blendSpaceVertex->Offset.x, blendSpaceVertex->Offset.y);
		}
		CDT::RemoveDuplicates(vertices);
		CDT::Triangulation<float> triangulation;
		triangulation.insertVertices(vertices);
		triangulation.eraseSuperTriangle();

		for (auto triangle : triangulation.triangles)
		{
			auto v0 = triangulation.vertices[triangle.vertices[0]];
			auto v1 = triangulation.vertices[triangle.vertices[1]];
			auto v2 = triangulation.vertices[triangle.vertices[2]];
			ImVec2 p0 = (ImVec2{ v0.x, v0.y } - blendSpace->Offset) / blendSpace->Scale;
			ImVec2 p1 = (ImVec2{ v1.x, v1.y } - blendSpace->Offset) / blendSpace->Scale;
			ImVec2 p2 = (ImVec2{ v2.x, v2.y } - blendSpace->Offset) / blendSpace->Scale;
			ImGui::GetWindowDrawList()->AddTriangle(p0, p1, p2, Colors::Theme::muted);
			ImGui::GetWindowDrawList()->AddTriangleFilled(p0, p1, p2, Colors::Theme::selectionMuted);
		}

#if 0
		// DEBUGGING: draw mouse position barycentric coords w.r.t the first triangle
		auto mousePos = ImGui::GetMousePos();
		auto blendSpaceMousePos = blendSpace->Offset + blendSpace->Scale * mousePos;
		for (auto triangle : triangulation.triangles)
		{
			auto i0 = triangle.vertices[0];
			auto i1 = triangle.vertices[1];
			auto i2 = triangle.vertices[2];

			auto v0 = triangulation.vertices[i0];
			auto v1 = triangulation.vertices[i1];
			auto v2 = triangulation.vertices[i2];

			auto det = ((v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y));
			auto u = ((v1.y - v2.y) * (blendSpaceMousePos.x - v2.x) + (v2.x - v1.x) * (blendSpaceMousePos.y - v2.y)) / det;
			auto v = ((v2.y - v0.y) * (blendSpaceMousePos.x - v2.x) + (v0.x - v2.x) * (blendSpaceMousePos.y - v2.y)) / det;
			auto w = 1.0f - u - v;

			std::string coords = std::format("({:.2f}, {:.2f}, {:.2f})", u, v, w);
			ImVec2 coordsSize = ImGui::CalcTextSize(coords.c_str());
			ImGui::SetCursorPos(mousePos + ImVec2{ -coordsSize.x / 2.0f, coordsSize.y + GImGui->Style.FramePadding.y });
			ImGui::TextUnformatted(coords.c_str());

			break;
		}
#endif
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
		if (UI::PropertyAssetReference<SkeletonAsset>("Skeleton", skeleton, "", &error, settings) && (error == UI::PropertyAssetReferenceError::None))
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
				HZ_CORE_ASSERT(selectedTransition);
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


	bool AnimationGraphEditor::PropertyBone(Pin* pin, bool& openBonePopup)
	{
		bool modified = false;

		std::string_view selectedBone = pin->Value["Value"].getString();
		int selectedBoneIndex = -1;

		s_SelectedBoneContext.BoneTokens.clear();

		int i = 0;
		s_SelectedBoneContext.BoneTokens.push_back({ "root", i });
		if (selectedBone == "root")
			selectedBoneIndex = i;
		++i;

		auto* model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		AssetHandle skeletonHandle = model->GetSkeletonHandle();
		auto skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(skeletonHandle);
		if (skeletonAsset)
		{
			s_SelectedBoneContext.BoneTokens.reserve(skeletonAsset->GetSkeleton().GetBoneNames().size() + 1);
			for (auto& boneName : skeletonAsset->GetSkeleton().GetBoneNames())
			{
				s_SelectedBoneContext.BoneTokens.push_back({ boneName, i });
				if (boneName == selectedBone)
					selectedBoneIndex = i;
				++i;
			}
		}

		UI::ScopedColourStack colors(
			ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_FrameBg),
			ImGuiCol_Text, selectedBoneIndex == -1? Colors::Theme::textError : ImGui::GetColorU32(ImGuiCol_Text)
		);
		float bestWidth = std::max(ImGui::CalcTextSize(selectedBone.data()).x, NodeEditorDraw::GetBestWidthForEnumCombo(s_SelectedBoneContext.BoneTokens));
		ImGui::SetNextItemWidth(bestWidth + ImGui::GetFrameHeightWithSpacing());
		if (UI::FakeComboBoxButton("##fakeCombo", selectedBone.data()))
		{
			openBonePopup = true;
			s_SelectedBoneContext.PinID = pin->ID;
			s_SelectedBoneContext.SelectedBone = selectedBone;
		}

		return modified;
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
			case AG::Types::EPinType::Vec3:             modified = NodeEditorDraw::PropertyVec3(pin->Name, pin->Value); break;
			case AG::Types::EPinType::AnimationAsset:   modified = NodeEditorDraw::PropertyAsset<AnimationAsset, AssetType::Animation>(pin->Value, pin, context.OpenAssetPopup); break;
			case AG::Types::EPinType::String:           modified = NodeEditorDraw::PropertyString(pin->Value); break;
			case AG::Types::EPinType::SkeletonAsset:    modified = NodeEditorDraw::PropertyAsset<SkeletonAsset, AssetType::Skeleton>(pin->Value, pin, context.OpenAssetPopup); break;
			case AG::Types::EPinType::Enum:
			{
				const int enumValue = pin->Value["Value"].get<int>();

				// TODO: JP. not ideal to do this callback style value assignment, should make it more generic.
				auto constructEnumValue = [](int selected) { return ValueFrom(AnimationGraph::Types::TEnum{ selected }); };

				modified = NodeEditorDraw::PropertyEnum(enumValue, pin, context.OpenEnumPopup, constructEnumValue);
				break;
			}
			case AG::Types::EPinType::Bone:             modified = PropertyBone(pin, context.OpenEnumPopup); break;
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
			HZ_CORE_ASSERT(false);
			return nullptr;
		}

		auto* model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		Node* transitionNode = model->CreateTransitionNode();
		if (transitionNode)
		{
			model->EnsureSubGraph(transitionNode);

			Node* startNode = GetModel()->FindNode(startNodeID);
			Node* endNode = GetModel()->FindNode(endNodeID);
			HZ_CORE_ASSERT(startNode && endNode);

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

		for (auto id : m_SelectedTransitionNodes)
		{
			m_TransitionsToDelete.insert(id);
		}

		for (auto id : m_SelectedBlendSpaceVertices)
		{
			m_BlendSpaceVerticesToDelete.insert(id);
		}
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
					HZ_CORE_ASSERT(false);
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

					HZ_CORE_ASSERT(newStartPin && newEndPin);

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
			HZ_CORE_ASSERT(newNode);

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


	void AnimationGraphEditor::CheckContextMenus()
	{
		// Check whether any context menus should be shown.

		if (ed::ShowNodeContextMenu(&m_State->ContextNodeId))
		{
			if (!ed::IsNodeSelected(m_State->ContextNodeId))
			{
				ed::SelectNode(m_State->ContextNodeId, ImGui::IsKeyDown(ImGuiKey_LeftCtrl));
			}
			ImGui::OpenPopup("Node Context Menu");
		}
		else if (m_State->HoveredTransition && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			if (!IsTransitionSelected(m_State->HoveredTransition))
			{
				ed::ClearSelection();
				SelectTransition(m_State->HoveredTransition, ImGui::IsKeyDown(ImGuiKey_LeftCtrl));
			}
			m_State->ContextNodeId = ed::NodeId(m_State->HoveredTransition);
			ImGui::OpenPopup("Node Context Menu");
		}
		else if (m_State->HoveredVertex && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			if (!IsBlendSpaceVertexSelected(m_State->HoveredVertex))
			{
				ed::ClearSelection();
				SelectBlendSpaceVertex(m_State->HoveredVertex, ImGui::IsKeyDown(ImGuiKey_LeftCtrl));
			}
			m_State->ContextNodeId = ed::NodeId(m_State->HoveredVertex);
			ImGui::OpenPopup("Node Context Menu");
		}
		else if (ed::ShowPinContextMenu(&m_State->ContextPinId))
		{
			ImGui::OpenPopup("Pin Context Menu");
		}
		else if (ed::ShowLinkContextMenu(&m_State->ContextLinkId))
		{
			if (!ed::IsLinkSelected(m_State->ContextLinkId))
			{
				ed::SelectLink(m_State->ContextLinkId, ImGui::IsKeyDown(ImGuiKey_LeftCtrl));
			}
			ImGui::OpenPopup("Link Context Menu");
		}
		else if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopup) && ed::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
			m_State->NewNodePopupOpening = true;
			m_State->NewNodeLinkPinId = 0;
		}
	}


	void AnimationGraphEditor::DrawNodeContextMenu(Node* node)
	{
		auto selectedNodeIDs = GetSelectedNodeIDs();

		if (selectedNodeIDs.empty() && m_SelectedTransitionNodes.empty() && m_SelectedBlendSpaceVertices.empty())
		{
			return;
		}

		if (ImGui::MenuItem("Delete"))
		{
			DeleteSelection();
		}

		if (selectedNodeIDs.empty() && m_SelectedBlendSpaceVertices.empty())
		{
			return;
		}

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
		{
			CopySelection();
		}

		if (ImGui::MenuItem("Paste"))
		{
			ed::ClearSelection();
			auto newNodeIDs = PasteSelection(m_CopiedNodeIDs, ed::ScreenToCanvas(ImGui::GetMousePos()));
			for (auto newNodeID : newNodeIDs)
			{
				ed::SelectNode(ed::NodeId(newNodeID));
			}
		}

		// If the selected node is a quick-state, then offer a "promote to state" option
		bool offerPromote = false;
		auto* model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		auto currentPath = model->GetCurrentPath();
		if (!currentPath.empty() && (currentPath.back()->GetTypeID() == AG::Types::ENodeType::StateMachine))
		{
			for (auto selectedNodeID : selectedNodeIDs)
			{
				auto* selectedNode = model->FindNode(selectedNodeID);
				if (selectedNode && selectedNode->GetTypeID() == AG::Types::QuickState)
				{
					offerPromote = true;
					break;
				}
			}
		}

		if (offerPromote)
		{
			ImGui::Separator();

			if (ImGui::MenuItem("Promote QuickState to State"))
			{
				for (auto selectedNodeID : selectedNodeIDs)
				{
					auto* selectedNode = model->FindNode(selectedNodeID);
					if (selectedNode && selectedNode->GetTypeID() == AG::Types::QuickState)
					{
						model->PromoteQuickStateToState(selectedNode);
					}
				}
			}
		}
	}


	void AnimationGraphEditor::DrawBackgroundContextMenu(bool& isNewNodePoppedUp, ImVec2& newNodePosition)
	{
		auto model = static_cast<AnimationGraphNodeEditorModel*>(GetModel());
		auto currentPath = model->GetCurrentPath();
		if (currentPath.empty() || (currentPath.back()->GetTypeID() != AG::Types::ENodeType::BlendSpace))
		{
			return IONodeGraphEditor::DrawBackgroundContextMenu(isNewNodePoppedUp, newNodePosition);
		}

		isNewNodePoppedUp = true;
		newNodePosition = ed::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());
		Node* node = nullptr;
		Ref<AnimationAsset> animationAsset = nullptr;
		AssetHandle thing;
		ImVec2 size = ImVec2(200.0f, 200.0f);

		// Search widget
		static bool grabFocus = true;
		static std::string searchString;
		if (ImGui::GetCurrentWindow()->Appearing)
		{
			grabFocus = true;
			searchString.clear();
		}
		UI::ShiftCursorX(4.0f);
		UI::ShiftCursorY(2.0f);
		UI::Widgets::SearchWidget(searchString, "Search Assets...", &grabFocus);
		{
			UI::ScopedColourStack headerColours(
				ImGuiCol_Header, IM_COL32(255, 255, 255, 0),
				ImGuiCol_HeaderActive, IM_COL32(45, 46, 51, 255),
				ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 80)
			);

			UI::ShiftCursorY(GImGui->Style.WindowPadding.y + 2.0f);
			if (ImGui::BeginChild("##new_node_scroll_list", ImVec2(0.0f, 350.0f)))
			{
				for (const auto& [path, metadata] : Project::GetEditorAssetManager()->GetAssetRegistry())
				{
					if (metadata.Type != AssetType::Animation)
						continue;

					const std::string assetName = metadata.FilePath.stem().string();

					if (!searchString.empty() && !UI::IsMatchingSearch(assetName, searchString))
						continue;

					if (ImGui::MenuItem(assetName.c_str()))
					{
						node = model->CreateNode("BlendSpace", "Blend Space Vertex");
						node->Inputs[0]->Value = Utils::CreateAssetHandleValue<AssetType::Animation>(metadata.Handle);
					}
				}
			}
			ImGui::EndChild();
		}

		if (node)
		{
			m_State->CreateNewNode = false;

			auto blendSpace = static_cast<AG::Types::Node*>(currentPath.back());
			ImVec2 snappedMousePos = newNodePosition;
			ImVec2 blendSpaceMousePos = blendSpace->Offset + blendSpace->Scale * snappedMousePos;

			// snap to integral blend space coordinates, unless ctrl is held
			if (!ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && !ImGui::IsKeyDown(ImGuiKey_RightCtrl))
			{
				blendSpaceMousePos = ImFloor(blendSpaceMousePos + ImVec2(0.5f, 0.5f));
				snappedMousePos = (blendSpaceMousePos - blendSpace->Offset) / blendSpace->Scale;
			}
			// We have to store blend space vertex position additionally to editor node position
			// because editor node position is stored as integer (and so loses precision)
			static_cast<AG::Types::Node*>(node)->Offset = blendSpaceMousePos;
			ed::SetNodePosition(ed::NodeId(node->ID), snappedMousePos);

			if (auto* startPin = GetModel()->FindPin(m_State->NewNodeLinkPinId))
			{
				auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;
				HZ_CORE_ASSERT(!pins.empty());
				for (auto& pin : pins)
				{
					if (GetModel()->CanCreateLink(startPin, pin))
					{
						auto endPin = pin;
						if (startPin->Kind == PinKind::Input)
							std::swap(startPin, endPin);

						GetModel()->CreateLink(startPin, endPin);
						break;
					}
				}
			}

			if (onNewNodeFromPopup)
				onNewNodeFromPopup(node);

			ImGui::CloseCurrentPopup();
		}
		m_State->NewNodePopupOpening = false;
	}


	void AnimationGraphEditor::DrawDeferredComboBoxes(PinPropertyContext& pinContext)
	{
		ed::Suspend();
		{
			/// Bone combo box
			if (pinContext.OpenEnumPopup && GetModel()->FindPin(s_SelectedBoneContext.PinID))
			{
				ImGui::OpenPopup("##BonePopup");
				pinContext.OpenEnumPopup = false;
			}

			if (auto pin = GetModel()->FindPin(s_SelectedBoneContext.PinID))
			{
				bool changed = false;

				ImGui::SetNextWindowSize({ NodeEditorDraw::GetBestWidthForEnumCombo(s_SelectedBoneContext.BoneTokens), 0.0f });
				if (UI::BeginPopup("##BonePopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
				{
					for (const auto& token : s_SelectedBoneContext.BoneTokens)
					{
						ImGui::PushID(token.name.data());
						const bool itemSelected = (token.name == s_SelectedBoneContext.SelectedBone);
						if (ImGui::Selectable(token.name.data(), itemSelected))
						{
							changed = true;
							s_SelectedBoneContext.SelectedBone = token.name;
						}
						if (itemSelected)
							ImGui::SetItemDefaultFocus();
						ImGui::PopID();
					}
					UI::EndPopup();
				}

				if (changed)
				{
					pin->Value = ValueFrom(AG::Types::TBone{std::string{s_SelectedBoneContext.SelectedBone}});
					if (GetModel()->onPinValueChanged)
						GetModel()->onPinValueChanged(pin->NodeID, pin->ID);
				}
			}
		}
		ed::Resume();
		IONodeGraphEditor::DrawDeferredComboBoxes(pinContext);
	}

}
