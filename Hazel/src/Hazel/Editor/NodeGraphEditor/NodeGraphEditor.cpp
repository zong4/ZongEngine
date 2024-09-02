#include "hzpch.h"
#include "NodeGraphEditor.h"

#include "NodeEditorModel.h"
#include "NodeGraphEditorContext.h"
#include "NodeGraphUtils.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Core/Input.h"
#include "Hazel/Editor/EditorResources.h"
#include "Hazel/ImGui/Colors.h"
#include "Hazel/ImGui/ImGuiUtilities.h"
#include "Hazel/ImGui/ImGuiWidgets.h"
#include "Hazel/Vendor/imgui-node-editor/widgets.h"

#include <choc/text/choc_StringUtilities.h>
#include <imgui-node-editor/builders.h>
#include <imgui-node-editor/imgui_node_editor_internal.h>

namespace ed = ax::NodeEditor;
namespace utils = ax::NodeEditor::Utilities;
using NodeBuilder = utils::BlueprintNodeBuilder;

static ax::NodeEditor::Detail::EditorContext* GetEditorContext()
{
	return (ax::NodeEditor::Detail::EditorContext*)(ed::GetCurrentEditor());
}

namespace Hazel
{

	struct EnumPinContext
	{
		UUID PinID = 0;
		int SelectedValue = 0;

		/*	Implementation might want to store enum value as a member of an object, or as int32.
			This callback allows implementation to construct choc::Value from selected item on change.
		*/
		std::function<choc::value::Value(int selected)> ConstructValue;
	};

	static EnumPinContext s_SelectedEnumContext;


	//=================================================================
	/// NodeEditor Draw Utilities
#pragma region Node Editor Draw Utilities

	float NodeEditorDraw::GetBestWidthForEnumCombo(const std::vector<Token>& enumTokens)
	{
		float width = 40.0f;
		for (const auto& token : enumTokens)
		{
			const float strWidth = ImGui::CalcTextSize(token.name.data()).x;
			if (strWidth > width)
				width = strWidth;
		}
		return width + GImGui->Style.WindowPadding.x * 2.0f;
	}


	bool NodeEditorDraw::PropertyBool(choc::value::Value& value)
	{
		bool modified = false;

		bool pinValue = !value.isVoid() ? value.get<bool>() : false;

		if (UI::Checkbox("##bool", &pinValue))
		{
			value = choc::value::createBool(pinValue);
			modified = true;
		}

		return modified;
	}


	bool NodeEditorDraw::PropertyFloat(choc::value::Value& value, const char* format)
	{
		bool modified = false;

		float pinValue = !value.isVoid() ? value.get<float>() : 0.0f;

		char v_str[64];
		ImFormatString(v_str, IM_ARRAYSIZE(v_str), format, pinValue);
		const float valueWidth = ImGui::CalcTextSize(v_str).x + GImGui->Style.FramePadding.y * 2.0f;
		ImGui::PushItemWidth(glm::max(valueWidth, 40.0f));

		if (UI::DragFloat("##floatIn", &pinValue, 0.01f, 0.0f, 0.0f, format))
		{
			value = choc::value::createFloat32(pinValue);
			modified = true;
		}

		ImGui::PopItemWidth();

		return modified;
	}


	bool NodeEditorDraw::PropertyInt(choc::value::Value& value, const char* format)
	{
		bool modified = false;

		int pinValue = !value.isVoid() ? value.get<int>() : 0;

		char v_str[64];
		ImFormatString(v_str, IM_ARRAYSIZE(v_str), format, pinValue);
		const float valueWidth = ImGui::CalcTextSize(v_str).x + GImGui->Style.FramePadding.y * 2.0f;
		ImGui::PushItemWidth(glm::max(valueWidth, 30.0f));

		if (UI::DragInt32("##int", &pinValue, 1.0f, 0, 0, format))
		{
			value = choc::value::createInt32(pinValue);
			modified = true;
		}

		ImGui::PopItemWidth();

		return modified;
	}


	bool NodeEditorDraw::PropertyVec3(std::string_view label, choc::value::Value& value, const char* format)
	{
		bool modified = false;
		bool manuallyEdited = false;

		glm::vec3 pinValue = CustomValueTo<glm::vec3>(value).value_or(glm::vec3{});

		char v_str[64];
		ImFormatString(v_str, IM_ARRAYSIZE(v_str), format, 9999.99f);
		float valueWidth = ImGui::CalcTextSize(v_str).x;
		ImFormatString(v_str, IM_ARRAYSIZE(v_str), format, pinValue.x);
		valueWidth = glm::max(valueWidth, ImGui::CalcTextSize(v_str).x);
		ImFormatString(v_str, IM_ARRAYSIZE(v_str), format, pinValue.y);
		valueWidth = glm::max(valueWidth, ImGui::CalcTextSize(v_str).x);
		ImFormatString(v_str, IM_ARRAYSIZE(v_str), format, pinValue.z);
		valueWidth = glm::max(valueWidth, ImGui::CalcTextSize(v_str).x);
		valueWidth = (valueWidth + 4.0f + GImGui->Font->FontSize + 6.0f) * 3.0f;

		if (UI::Widgets::EditVec3(label, ImVec2(valueWidth, ImGui::GetFrameHeightWithSpacing() + 8.0f), 0.0f, manuallyEdited, pinValue, UI::VectorAxis::None, 1.0f, glm::zero<glm::vec3>(), glm::zero<glm::vec3>(), format))
		{
			value = ValueFrom(pinValue);
			modified = true;
		}

		return modified;
	}

	bool NodeEditorDraw::PropertyString(choc::value::Value& value)
	{
		bool modified = false;

		std::string valueStr;

		if (!value.isVoid())
		{
			valueStr = value.get<std::string>();
		}

		ImGui::PushItemWidth(100.0f);

		const auto inputTextFlags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
		UI::InputText("##edit", &valueStr, inputTextFlags);

		ImGui::PopItemWidth();

		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			value = choc::value::createString(valueStr);
			modified = true;
		}

		return modified;
	}


	bool NodeEditorDraw::PropertyEnum(int enumValue, Pin* pin, bool& openEnumPopup, std::function<choc::value::Value(int selected)> constructValue)
	{
		HZ_CORE_VERIFY(pin);

		bool modified = false;

		int pinValue = enumValue;

		std::optional<const std::vector<Token>*> optTokens = pin->EnumTokens;
		if (optTokens.has_value() && optTokens.value() != nullptr)
		{
			HZ_CORE_ASSERT(pinValue >= 0);
			UI::ScopedColour bg(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_FrameBg));

			const auto& tokens = *optTokens.value();

			ImGui::SetNextItemWidth(GetBestWidthForEnumCombo(tokens));
			if (UI::FakeComboBoxButton("##fakeCombo", tokens[pinValue].name.data()))
			{
				openEnumPopup = true;
				s_SelectedEnumContext = { pin->ID, pinValue };
				s_SelectedEnumContext.ConstructValue = constructValue;
			}
		}

		return modified;
	}
#pragma endregion


	NodeGraphEditorBase::NodeGraphEditorBase(const char* id)
		: AssetEditor(id)
	{
		m_State = new ContextState;
		m_CanvasName = "Canvas##" + m_Id;
		m_NodeBuilderTexture = EditorResources::TranslucentGradientTexture;
	}

	NodeGraphEditorBase::~NodeGraphEditorBase() {
		delete m_State;
	}


	void NodeGraphEditorBase::SetAsset(const Ref<Asset>& asset)
	{
		const AssetMetadata& metadata = Project::GetEditorAssetManager()->GetMetadata(asset->Handle);
		SetTitle(metadata.FilePath.stem().string());
	}


	std::vector<UUID> NodeGraphEditorBase::GetSelectedNodeIDs() const
	{
		std::vector<ed::NodeId> selectedNodes;
		selectedNodes.resize(ed::GetSelectedObjectCount());
		int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
		selectedNodes.resize(nodeCount);

		std::vector<UUID> ids;
		for (const auto& nodeID : selectedNodes)
			ids.push_back(nodeID.Get());

		return ids;
	}


	bool NodeGraphEditorBase::InitializeEditor()
	{
		ed::Config config;

		config.CanvasSizeMode = ed::CanvasSizeMode::CenterOnly;
		config.SettingsFile = nullptr;
		config.UserPointer = this;

		// Limiting to 1:1 doesn't allow sufficient zoom on a high DPI monitor.
		// Can revisit this if/when Hazelnut properly supports high DPI.
		//
		//// Set up custom zeoom levels
		//{
		//	// Limitting max zoom in to 1:1
		//	static constexpr float zooms[] = { 0.1f, 0.15f, 0.20f, 0.25f, 0.33f, 0.5f, 0.75f, 1.0f };
		//	for (uint32_t i = 0; i < sizeof(zooms) / sizeof(*zooms); ++i)
		//		config.CustomZoomLevels.push_back(zooms[i]);
		//}

		// Save graph state
		config.SaveSettings = [](const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
		{
			auto* nodeEditor = static_cast<NodeGraphEditorBase*>(userPointer);
			return nodeEditor->GetModel()->SaveGraphState(data, size);
		};

		// Load graph state
		config.LoadSettings = [](char* data, void* userPointer) -> size_t
		{
			auto* nodeEditor = static_cast<NodeGraphEditorBase*>(userPointer);
			const std::string& graphState = nodeEditor->GetModel()->LoadGraphState();
			if (data)
			{
				memcpy(data, graphState.data(), graphState.size());
			}
			return graphState.size();
		};

		// Restore node location
		config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
		{
			auto* nodeEditor = static_cast<NodeGraphEditorBase*>(userPointer);

			if (!nodeEditor->GetModel())
				return 0;

			auto node = nodeEditor->GetModel()->FindNode(UUID(nodeId.Get()));
			if (!node)
				return 0;


			if (data != nullptr)
				memcpy(data, node->State.data(), node->State.size());
			return node->State.size();
		};

		// Store node location
		config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
		{
			auto* nodeEditor = static_cast<NodeGraphEditorBase*>(userPointer);

			if (!nodeEditor->GetModel())
				return false;

			auto node = nodeEditor->GetModel()->FindNode(UUID(nodeId.Get()));
			if (!node)
				return false;

			node->State.assign(data, size);

			////self->TouchNode(nodeId);

			return true;
		};

		// NOTE: ed::CreateEditor takes a deep copy of the passed in config
		m_Editor = ed::CreateEditor(&config);
		ed::SetCurrentEditor(m_Editor);

		InitializeEditorStyle(ed::GetStyle());

		return true;
	}


	void NodeGraphEditorBase::Render()
	{
		if (!GetModel())
			return;

		if (!m_Editor || !m_Initialized)
		{
			m_Initialized = InitializeEditor();
			m_FirstFrame = true;
		}

		m_MainDockID = ImGui::DockSpace(ImGui::GetID(m_Id.c_str()), ImVec2(0, 0), ImGuiDockNodeFlags_AutoHideTabBar);

		UI::ScopedStyle stylePadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 2.0f));

		m_WindowClass.ClassId = ImGui::GetID(m_Id.c_str());
		m_WindowClass.DockingAllowUnclassed = false;

		ImGui::SetNextWindowDockID(m_MainDockID, ImGuiCond_Always);
		ImGui::SetNextWindowClass(GetWindowClass());

		if (ImGui::Begin(m_CanvasName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse))
		{
			// Don't render graph on first frame.
			// This gives ImGui a chance to layout the windows before we start drawing the graph.
			// This is important as drawing the graph into incorrectly sized window causes
			// the graph to save its settings with incorrect visible region.
			if (!m_FirstFrame)
			{
				auto [editorMin, editorMax] = DrawGraph("Node Editor");

				// Draw on top of canvas for subclasses
				OnRenderOnCanvas(editorMin, editorMax);
			}
		}
		ImGui::End(); // Canvas

		if (m_State->EditNodeId)
		{
			Node* node = GetModel()->FindNode(m_State->EditNodeId);
			if (node)
			{
				if (auto onEditNode = GetModel()->onEditNode)
				{
					onEditNode(node);
					m_FirstFrame = true;
				}
			}
			m_State->EditNodeId = 0;
		}

		// Draw windows of subclasses
		OnRender();
		m_FirstFrame = false;
	}


	bool NodeGraphEditorBase::DeactivateInputIfDragging(const bool wasActive)
	{
		if (ImGui::IsItemActive() && !wasActive)
		{
			m_State->DraggingInputField = true;
			ed::EnableShortcuts(false);
			return true;
		}
		else if (!ImGui::IsItemActive() && wasActive)
		{
			m_State->DraggingInputField = false;
			ed::EnableShortcuts(true);
			return false;
		}
		return wasActive;
	}


	void NodeGraphEditorBase::OnClose()
	{
		if (auto* model = GetModel(); model)
		{
			if (model->onStop)
			{
				model->onStop(true);
			}
			model->SaveAll();
		}

		if (m_Editor)
		{
			// Destroy editor causes settings to be saved.
			// However, saving the settings at this point results in bung settings.
			// Disable saving.
			// (the settings are saved every time the canvas is panned or zoomed anyway, so 
			// this is not missing out anything)
			//
			// imgui_node_editor api does not make this easy...
			ed::SetCurrentEditor(m_Editor);
			const_cast<ax::NodeEditor::Detail::Config&>(GetEditorContext()->GetConfig()).SaveSettings = nullptr;

			ed::DestroyEditor(m_Editor);
			m_Editor = nullptr;
		}
	}


	void NodeGraphEditorBase::LoadSettings()
	{
		GetEditorContext()->LoadSettings();
	}


	void NodeGraphEditorBase::InitializeEditorStyle(ax::NodeEditor::Style& editorStyle)
	{
		// Style
		editorStyle.NodePadding = { 0.0f, 4.0f, 0.0f, 0.0f }; // This mostly affects Comment nodes
		editorStyle.NodeRounding = 3.0f;
		editorStyle.NodeBorderWidth = 1.0f;
		editorStyle.HoveredNodeBorderWidth = 2.0f;
		editorStyle.SelectedNodeBorderWidth = 3.0f;
		editorStyle.PinRounding = 2.0f;
		editorStyle.PinBorderWidth = 0.0f;
		editorStyle.LinkStrength = 80.0f;
		editorStyle.ScrollDuration = 0.35f;
		editorStyle.FlowMarkerDistance = 30.0f;
		editorStyle.FlowDuration = 2.0f;
		editorStyle.GroupRounding = 0.0f;
		editorStyle.GroupBorderWidth = 0.0f;
		editorStyle.GridSnap = 8.0f;

		editorStyle.HighlightConnectedLinks = 1.0f;

		// Extra (for now just using defaults)
		editorStyle.SnapLinkToPinDir = 0.0f;
		editorStyle.PivotAlignment = ImVec2(0.5f, 0.5f);    // This one is changed in Draw
		editorStyle.PivotSize = ImVec2(0.0f, 0.0f);         // This one is changed in Draw
		editorStyle.PivotScale = ImVec2(1, 1);              // This one is not used
		editorStyle.PinCorners = ImDrawFlags_RoundCornersAll;
		editorStyle.PinRadius = 0.0f;       // This one is changed in Draw
		editorStyle.PinArrowSize = 0.0f;    // This one is changed in Draw
		editorStyle.PinArrowWidth = 0.0f;   // This one is changed in Draw

		// Colours
		editorStyle.Colors[ed::StyleColor_Bg] = ImColor(23, 24, 28, 200);
		editorStyle.Colors[ed::StyleColor_Grid] = ImColor(21, 21, 21, 255);// ImColor(60, 60, 60, 40);
		editorStyle.Colors[ed::StyleColor_NodeBg] = ImColor(31, 33, 38, 255);
		editorStyle.Colors[ed::StyleColor_NodeBorder] = ImColor(51, 54, 62, 140);
		editorStyle.Colors[ed::StyleColor_HovNodeBorder] = ImColor(60, 180, 255, 150);
		editorStyle.Colors[ed::StyleColor_SelNodeBorder] = ImColor(255, 225, 135, 255);
		editorStyle.Colors[ed::StyleColor_NodeSelRect] = ImColor(5, 130, 255, 64);
		editorStyle.Colors[ed::StyleColor_NodeSelRectBorder] = ImColor(5, 130, 255, 128);
		editorStyle.Colors[ed::StyleColor_HovLinkBorder] = ImColor(60, 180, 255, 255);
		editorStyle.Colors[ed::StyleColor_SelLinkBorder] = ImColor(255, 225, 135, 255);
		editorStyle.Colors[ed::StyleColor_LinkSelRect] = ImColor(5, 130, 255, 64);
		editorStyle.Colors[ed::StyleColor_LinkSelRectBorder] = ImColor(5, 130, 255, 128);
		editorStyle.Colors[ed::StyleColor_PinRect] = ImColor(60, 180, 255, 0);
		editorStyle.Colors[ed::StyleColor_PinRectBorder] = ImColor(60, 180, 255, 0);
		editorStyle.Colors[ed::StyleColor_Flow] = ImColor(255, 128, 64, 255);
		editorStyle.Colors[ed::StyleColor_FlowMarker] = ImColor(255, 128, 64, 255);
		editorStyle.Colors[ed::StyleColor_GroupBg] = ImColor(255, 255, 255, 30);
		editorStyle.Colors[ed::StyleColor_GroupBorder] = ImColor(0, 0, 0, 0);

		editorStyle.Colors[ed::StyleColor_HighlightLinkBorder] = ImColor(255, 255, 255, 140);
	}


	void NodeGraphEditorBase::EnsureWindowIsDocked(ImGuiWindow* childWindow)
	{
		if (childWindow->DockNode && childWindow->DockNode->ParentNode)
			m_DockIDs[childWindow->ID] = childWindow->DockNode->ParentNode->ID;

		//? For some reason IsAnyMouseDown is not reporting correctly anymore
		//? So for now "auto docking" is disabled
		if (!childWindow->DockIsActive && !ImGui::IsAnyMouseDown() && !childWindow->DockNode && !childWindow->DockNodeIsVisible)
		{
			if (!m_DockIDs[childWindow->ID])
				m_DockIDs[childWindow->ID] = m_MainDockID;

			ImGui::SetWindowDock(childWindow, m_DockIDs[childWindow->ID], ImGuiCond_Always);
		}
	}


	ImColor NodeGraphEditorBase::GetIconColor(int pinTypeID) const
	{
		return GetModel()->GetIconColor(pinTypeID);
	};


	bool NodeGraphEditorBase::DrawPinPropertyEdit(PinPropertyContext& context)
	{
		bool modified = false;

		Pin* pin = context.pin;

		switch ((EPinType)pin->GetType())
		{
			case EPinType::Bool:    modified = NodeEditorDraw::PropertyBool(pin->Value); break;
			case EPinType::Int:     modified = NodeEditorDraw::PropertyInt(pin->Value); break;
			case EPinType::Float:   modified = NodeEditorDraw::PropertyFloat(pin->Value); break;
			case EPinType::String:  modified = NodeEditorDraw::PropertyString(pin->Value); break;
			case EPinType::Enum:    modified = NodeEditorDraw::PropertyEnum(pin->Value.get<int>(), pin, context.OpenEnumPopup,
				[](int selected) {return choc::value::createInt32(selected);}); break;
			default:
				HZ_CORE_ASSERT(false, "Unhandled pin type")
				break;
		}

		return modified;
	}


	std::pair<ImVec2, ImVec2> NodeGraphEditorBase::DrawGraph(const char* id)
	{
		// This ensures that we don't start selection rectangle when dragging Canvas window titlebar
		bool draggingCanvasTitlebar = false;
		if (ImGui::IsItemActive())
			draggingCanvasTitlebar = true;

		m_State->NewNodePopupOpening = false;
		PinPropertyContext pinContext{ nullptr, GetModel(), false, false };

		ed::Begin(id);
		{
			auto cursorTopLeft = ImGui::GetCursorScreenPos();
			DrawNodes(pinContext);
			DrawDeferredComboBoxes(pinContext);
			DrawLinks();

			if (ed::BeginDelete())
			{
				ed::LinkId linkId = 0;
				while (ed::QueryDeletedLink(&linkId))
				{
					if (ed::AcceptDeletedItem())
						GetModel()->RemoveLink(linkId.Get());
				}

				ed::NodeId nodeId = 0;
				while (ed::QueryDeletedNode(&nodeId))
				{
					if (ed::AcceptDeletedItem())
						GetModel()->RemoveNode(nodeId.Get());
				}
			}
			ed::EndDelete();

			ImGui::SetCursorScreenPos(cursorTopLeft);

			ed::Suspend();
			CheckContextMenus();
			ed::Resume();

			ed::Suspend();
			UI::ScopedStyleStack(
				ImGuiStyleVar_WindowPadding, ImVec2(8, 8),
				ImGuiStyleVar_FrameBorderSize, 0.0f
			);
			UI::ScopedColourStack popupColours(
				ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 80),
				ImGuiCol_Separator, IM_COL32(90, 90, 90, 255),
				ImGuiCol_Text, Colors::Theme::textBrighter
			);

			if (UI::BeginPopup("Node Context Menu"))
			{
				auto node = GetModel()->FindNode(m_State->ContextNodeId.Get());
				DrawNodeContextMenu(node);

				UI::EndPopup();
			}

			if (UI::BeginPopup("Pin Context Menu"))
			{
				auto pin = GetModel()->FindPin(m_State->ContextPinId.Get());
				DrawPinContextMenu(pin);

				UI::EndPopup();
			}

			if (UI::BeginPopup("Link Context Menu"))
			{
				auto link = GetModel()->FindLink(m_State->ContextLinkId.Get());
				DrawLinkContextMenu(link);

				UI::EndPopup();
			}

			bool isNewNodePoppedUp = false;
			static ImVec2 newNodePostion{ 0.0f, 0.0f };
			{
				UI::ScopedStyle scrollListStyle(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
				if (UI::BeginPopup("Create New Node"))
				{
					DrawBackgroundContextMenu(isNewNodePoppedUp, newNodePostion);
					auto* popupWindow = ImGui::GetCurrentWindow();
					auto shadowRect = ImRect(popupWindow->Pos, popupWindow->Pos + popupWindow->Size);
					UI::EndPopup();
					UI::DrawShadow(EditorResources::ShadowTexture, 14, shadowRect, 1.3f);
				}
				else
				{
					if (m_State->CreateNewNode && onNewNodePopupCancelled)
						onNewNodePopupCancelled();

					m_State->CreateNewNode = false;
				}
			}
			ed::Resume();

			if (isNewNodePoppedUp && m_State->NewNodeLinkPinId)
			{
				if (const auto* newNodeLinkPin = GetModel()->FindPin(m_State->NewNodeLinkPinId))
				{
					const float lineThickness = 2.0f;
					auto& creator = GetEditorContext()->GetItemCreator();
					if (creator.m_IsActive)
						creator.SetStyle(GetIconColor(newNodeLinkPin->GetType()), lineThickness);

					// this is needed to fix the case when if you move mouse fast when release dragged link,
					// the link end drops along the way behind the actual popup position
					auto& draggedPivot = newNodeLinkPin->Kind == PinKind::Output ? creator.m_lastEndPivot : creator.m_lastStartPivot;
					const ImVec2 offset = newNodePostion - draggedPivot.GetCenter();
					draggedPivot.Translate(offset);

					ed::DrawLastLink();
				}
			}
		}

		ed::End();

		if (onDragDropOntoCanvas)
		{
			if (ImGui::BeginDragDropTarget())
			{
				onDragDropOntoCanvas();
				ImGui::EndDragDropTarget();
			}
		}

		auto editorMin = ImGui::GetItemRectMin();
		auto editorMax = ImGui::GetItemRectMax();

		//// Draw current zoom level
		//{
		//	UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
		//	const ImVec2 zoomLabelPos({ editorMin.x + 20.0f, editorMax.y - 40.0f });
		//	const std::string zoomLabelText = "Zoom " + choc::text::floatToString(ed::GetCurrentZoom(), 1, true);
		//
		//	ImGui::GetWindowDrawList()->AddText(zoomLabelPos, IM_COL32(255, 255, 255, 60), zoomLabelText.c_str());
		//}

		UI::DrawShadowInner(EditorResources::ShadowTexture, 50, editorMin, editorMax, 0.3f, (editorMax.y - editorMin.y) / 4.0f);

		// A bit of a hack to prevent editor from dragging and selecting while dragging an input field
		// It still draws a dot where the Selection Action should be, but it's not too bad
		if (m_State->DraggingInputField || draggingCanvasTitlebar)
		{
			auto* editor = GetEditorContext();
			if (auto* action = editor->GetCurrentAction())
			{
				if (auto* dragAction = action->AsDrag())
					dragAction->m_Clear = true;
				if (auto* selectAction = action->AsSelect())
					selectAction->m_IsActive = false;
			}
		}
		return { editorMin, editorMax };
	}


	void NodeGraphEditorBase::DrawNodes(PinPropertyContext& pinContext)
	{
		utils::BlueprintNodeBuilder builder(UI::GetTextureID(m_NodeBuilderTexture), m_NodeBuilderTexture->GetWidth(), m_NodeBuilderTexture->GetHeight());

		// All nodes other than comments must be drawn first,
		// and then all comment nodes drawn at the end.
		// and then all comment nfaodes drawn at the end.
		// This is because comment nodes can act to group together
		// other nodes (and hence all other nodes must be drawn first)
		for (auto& node : GetModel()->GetNodes())
		{
			// When we do a copy/paste the pasted nodes have no position in the editor.
			// In this case, set the editor node pos from the position saved in the node state
			if((ed::GetNodePosition(ed::NodeId(node->ID)) == ImVec2(FLT_MAX, FLT_MAX)) && (!node->State.empty()))
				ed::SetNodePosition(ed::NodeId(node->ID), node->GetPosition());

			if (node->Type != NodeType::Comment)
				DrawNode(node, builder, pinContext);
		}

		for (auto& node : GetModel()->GetNodes())
		{
			if (node->Type == NodeType::Comment)
				DrawCommentNode(node);
		}
	}


	void NodeGraphEditorBase::DrawLinks()
	{
		// Links
		for (auto& link : GetModel()->GetLinks())
			ed::Link(ed::LinkId(link.ID), ed::PinId(link.StartPinID), ed::PinId(link.EndPinID), link.Color, 2.0f);

		m_AcceptingLinkPins = { nullptr, nullptr };

		// Draw dragging Link
		if (!m_State->CreateNewNode)
		{
			const ImColor draggedLinkColour = ImColor(255, 255, 255);
			const float lineThickness = 2.0f;
			if (ed::BeginCreate(draggedLinkColour, lineThickness))
			{
				auto showLabel = [](const char* label, ImColor color)
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
					auto size = ImGui::CalcTextSize(label);

					auto padding = ImGui::GetStyle().FramePadding;
					auto spacing = ImGui::GetStyle().ItemSpacing;

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

					auto rectMin = ImGui::GetCursorScreenPos() - padding;
					auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
					ImGui::TextUnformatted(label);
				};

				ed::PinId startPinId = 0, endPinId = 0;
				if (ed::QueryNewLink(&startPinId, &endPinId))
				{
					auto startPin = GetModel()->FindPin(startPinId.Get());
					auto endPin = GetModel()->FindPin(endPinId.Get());

					m_State->NewLinkPin = startPin ? startPin : endPin;

					if (startPin->Kind == PinKind::Input)
					{
						std::swap(startPin, endPin);
						std::swap(startPinId, endPinId);
					}

					if (startPin && endPin && (startPin != endPin))
					{
						NodeEditorModel::LinkQueryResult canConnect = GetModel()->CanCreateLink(startPin, endPin);
						if (canConnect)
						{
							m_AcceptingLinkPins.first = startPin;
							m_AcceptingLinkPins.second = endPin;
							showLabel("+ Create Link", ImColor(32, 45, 32, 180));

							// Set link color when attacking to a compatible pin
							const ImColor linkColor = GetIconColor(startPin->GetType());

							if (ed::AcceptNewItem(linkColor, 4.0f))
							{
								GetModel()->CreateLink(startPin, endPin);
							}
						}
						else
						{
							switch (canConnect.Reason)
							{
								case NodeEditorModel::LinkQueryResult::IncompatibleStorageKind:
								case NodeEditorModel::LinkQueryResult::IncompatiblePinKind:
								{
									showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
									break;
								}
								case NodeEditorModel::LinkQueryResult::IncompatibleType:
								{
									showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
									break;
								}
								case NodeEditorModel::LinkQueryResult::SamePin:
								case NodeEditorModel::LinkQueryResult::SameNode:
								{
									showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
									break;
								}
								case NodeEditorModel::LinkQueryResult::CausesLoop:
								{
									showLabel("x Connection causes loop", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
									break;
								}
								case NodeEditorModel::LinkQueryResult::InvalidStartPin:
								case NodeEditorModel::LinkQueryResult::InvalidEndPin:
								default:
									break;
							}
						}
					}
				}

				ed::PinId pinId = 0;
				if (ed::QueryNewNode(&pinId))
				{
					m_State->NewLinkPin = GetModel()->FindPin(pinId.Get());
					if (m_State->NewLinkPin)
						showLabel("+ Create Node", ImColor(32, 45, 32, 180));

					const ImColor draggedLinkColour = m_State->NewLinkPin ? GetIconColor(m_State->NewLinkPin->GetType()) : ImColor(255, 255, 255);
					const float lineThickness = 2.0f;

					if (ed::AcceptNewItem(draggedLinkColour, lineThickness))
					{
						m_State->CreateNewNode = true;
						m_State->NewNodeLinkPinId = pinId.Get();
						m_State->NewLinkPin = nullptr;

						ed::Suspend();
						ImGui::OpenPopup("Create New Node");
						m_State->NewNodePopupOpening = true;
						ed::Resume();
					}
				}
			}
			else
				m_State->NewLinkPin = nullptr;

			ed::EndCreate();
		}

		// Alt + Click to remove links connected to clicked pin
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyAlt)
		{
			auto hoveredPinId = (UUID)ed::GetHoveredPin().Get();
			if (hoveredPinId)
			{
				auto* model = GetModel();
				std::vector<UUID> links = model->GetAllLinkIDsConnectedToPin(hoveredPinId);
				model->RemoveLinks(links);
			}
		}
	}


	void NodeGraphEditorBase::DrawNodeHeader(Node* node, NodeBuilder& builder)
	{
		bool hasOutputDelegates = false;
		for (auto& output : node->Outputs)
			if (output->IsType(EPinType::DelegatePin))
				hasOutputDelegates = true;

		builder.Header(node->Color);
		{
			ImGui::Spring(0);
			UI::ScopedColour headerTextColor(ImGuiCol_Text, IM_COL32(210, 210, 210, 255));

			if (node->Type == NodeType::Subroutine)
			{
				static bool wasActive = false;

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

				ImVec2 textSize = ImGui::CalcTextSize((node->Name + "00").c_str());
				textSize.x = std::min(textSize.x, availableWidth - 16.0f);

				ImGui::PushItemWidth(textSize.x);
				{
					UI::ScopedColourStack stack(ImGuiCol_FrameBg, ImColor(0, 0, 0, 0), ImGuiCol_Border, IM_COL32_DISABLE);
					ImGui::InputText("##edit", &node->Description, inputTextFlags);
					DrawItemActivityOutline(UI::OutlineFlags_NoOutlineInactive);
				}
				ImGui::PopItemWidth();
				wasActive = DeactivateInputIfDragging(wasActive);
			}
			else
			{
				ImGui::TextUnformatted(node->Name.c_str());
			}

			if (m_ShowNodeIDs)
				ImGui::TextUnformatted(("(" + std::to_string(node->ID) + ")").c_str());
			if (m_ShowSortIndices)
				ImGui::TextUnformatted(("(sort index: " + std::to_string(node->SortIndex) + ")").c_str());
		}
		ImGui::Spring(1);

		const float nodeHeaderHeight = 18.0f;
		ImGui::Dummy(ImVec2(0, nodeHeaderHeight));

		// Delegate pin
		if (hasOutputDelegates)
		{
			ImGui::BeginVertical("delegates", ImVec2(0, nodeHeaderHeight));
			ImGui::Spring(1, 0);
			for (auto& output : node->Outputs)
			{
				if (output->IsType(EPinType::DelegatePin))
					continue;

				auto alpha = ImGui::GetStyle().Alpha;
				if (m_State->NewLinkPin && !GetModel()->CanCreateLink(m_State->NewLinkPin, output) && output != m_State->NewLinkPin)
				{
					alpha = alpha * (48.0f / 255.0f);
				}

				ed::BeginPin(ed::PinId(output->ID), ed::PinKind::Output);
				ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
				ed::PinPivotSize(ImVec2(0, 0));
				ImGui::BeginHorizontal((int)output->ID);
				UI::ScopedStyle alphaStyle(ImGuiStyleVar_Alpha, alpha);
				if (!output->Name.empty())
				{
					ImGui::TextUnformatted(output->Name.c_str());
					ImGui::Spring(0);
				}
				DrawPinIcon(output, GetModel()->IsPinLinked(output->ID), (int)(alpha * 255));
				ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
				ImGui::EndHorizontal();
				ed::EndPin();
			}
			ImGui::Spring(1, 0);
			ImGui::EndVertical();
			ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
		}
		else
		{
			ImGui::Spring(0);
		}

		builder.EndHeader();
	}


	void NodeGraphEditorBase::DrawNodeMiddle(Node* node, NodeBuilder& builder)
	{
		if (node->Type == NodeType::Simple)
		{
			builder.Middle();

			ImGui::Spring(1, 0);
			UI::ScopedColour textColour(ImGuiCol_Text, IM_COL32(210, 210, 210, 255));

			// TODO: handle simple nodes display icon properly
			if (const char* displayIcon = GetIconForSimpleNode(node->Name))
			{
				UI::ScopedColour textColour(ImGuiCol_Text, Colors::Theme::text);
				UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
				ImGui::TextUnformatted(displayIcon);
			}
			else
				ImGui::TextUnformatted(node->Name.c_str());

			if (m_ShowNodeIDs)
				ImGui::TextUnformatted(("(" + std::to_string(node->ID) + ")").c_str());

			ImGui::Spring(1, 0);
		}

		if (node->Type == NodeType::Subroutine)
		{
			builder.Middle();
			UI::ScopedColour textColour(ImGuiCol_Text, IM_COL32(210, 210, 210, 255));
			UI::ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(16, 8));
			UI::ScopedStyle frameRounding(ImGuiStyleVar_FrameRounding, 4.0f);
			if (ImGui::Button("Edit..."))
			{
				m_State->EditNodeId = node->ID;
			}
		}

	}

	void NodeGraphEditorBase::DrawNodeInputs(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext)
	{
		for (auto& input : node->Inputs)
		{
			bool pinValueChanged = false;

			auto alpha = ImGui::GetStyle().Alpha;
			if (m_State->NewLinkPin && !GetModel()->CanCreateLink(m_State->NewLinkPin, input) && input != m_State->NewLinkPin)
			{
				alpha = alpha * (48.0f / 255.0f);
			}

			builder.Input(ed::PinId(input->ID));
			UI::ScopedStyle alphaStyle(ImGuiStyleVar_Alpha, alpha);
			DrawPinIcon(input, GetModel()->IsPinLinked(input->ID), (int)(alpha * 255));
			ImGui::Spring(0);

			// Draw input pin name
			if ((node->Type != NodeType::Simple) && !input->Name.empty())
			{
				ImGui::TextUnformatted(input->Name.c_str());
				ImGui::Spring(0);
			}

			// TODO: implementation might want to draw property edit for linked pins as well(?)
			if (input->Storage != StorageKind::Array && !GetModel()->IsPinLinked(input->ID))
			{
				UI::ScopedStyle frameRounding(ImGuiStyleVar_FrameRounding, 2.5f);

				// TODO: handle implementation specific types
				PinPropertyContext context{ input, GetModel(), false, false };
				pinValueChanged = DrawPinPropertyEdit(context);

				if (context.OpenAssetPopup || context.OpenEnumPopup)
					pinContext = context;

				static bool wasActive = false;
				wasActive = DeactivateInputIfDragging(wasActive); //? this might not work, if it requires unique bool for each property type, or if property edit was not even drawn

				ImGui::Spring(0);
			}

			if (pinValueChanged && GetModel()->onPinValueChanged)
			{
				GetModel()->onPinValueChanged(node->ID, input->ID);
			}

			builder.EndInput();
		}
	}


	void NodeGraphEditorBase::DrawNodeOutputs(Node* node, NodeBuilder& builder)
	{
		for (auto& output : node->Outputs)
		{
			if ((node->Type != NodeType::Simple) && output->IsType(EPinType::DelegatePin))
				continue;

			auto alpha = ImGui::GetStyle().Alpha;
			if (m_State->NewLinkPin && !GetModel()->CanCreateLink(m_State->NewLinkPin, output) && output != m_State->NewLinkPin)
			{
				alpha = alpha * (48.0f / 255.0f);
			}

			UI::ScopedStyle alphaStyleOverride(ImGuiStyleVar_Alpha, alpha);

			builder.Output(ed::PinId(output->ID));

			// Draw output pin name
			if ((node->Type != NodeType::Simple) && !output->Name.empty() && output->Name != "Message")
			{
				ImGui::Spring(0);
				ImGui::TextUnformatted(output->Name.c_str());
			}
			ImGui::Spring(0);
			DrawPinIcon(output, GetModel()->IsPinLinked(output->ID), (int)(alpha * 255));
			builder.EndOutput();

			// TODO: implementation might want to draw property edit for outputs as well
		}
	}


	void NodeGraphEditorBase::DrawNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext)
	{
		HZ_CORE_ASSERT(node->Type != NodeType::Comment);
		builder.Begin(ed::NodeId(node->ID));

		if (node->Type != NodeType::Simple)
		{
			DrawNodeHeader(node, builder);
		}
		DrawNodeInputs(node, builder, pinContext);
		DrawNodeMiddle(node, builder);
		DrawNodeOutputs(node, builder);

		builder.End();

		int radius = EditorResources::ShadowTexture->GetHeight();// *1.4;
		UI::DrawShadow(EditorResources::ShadowTexture, radius);
	}


	void NodeGraphEditorBase::DrawCommentNode(Node* node)
	{
		HZ_CORE_ASSERT(node->Type == NodeType::Comment);

		const float commentAlpha = 0.75f;

		ed::PushStyleColor(ed::StyleColor_NodeBg, node->Color);
		ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 24));
		ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_DISABLE);
		ed::BeginNode(ed::NodeId(node->ID));
		{
			UI::ScopedStyle nodeStyle(ImGuiStyleVar_Alpha, commentAlpha);

			UI::ScopedID nodeID((int)node->ID);
			ImGui::BeginVertical("content");
			ImGui::BeginHorizontal("horizontal");
			{
				UI::ShiftCursorX(6.0f);
				UI::ScopedColour frameColour(ImGuiCol_FrameBg, IM_COL32(255, 255, 255, 0));
				UI::ScopedColour textColour(ImGuiCol_Text, IM_COL32(220, 220, 220, 255));
				UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);

				static bool wasActive = false;

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

				ImVec2 textSize = ImGui::CalcTextSize((node->Name + "00").c_str());
				textSize.x = std::min(textSize.x, availableWidth - 16.0f);

				ImGui::PushItemWidth(textSize.x);
				//ImGui::PushTextWrapPos(availableWidth); //? doesn't work
				{
					UI::ScopedStyle alpha(ImGuiStyleVar_Alpha, 1.0f);

					// Draw text shadow
					{
						UI::ScopedColour textShadow(ImGuiCol_Text, IM_COL32_BLACK);
						const auto pos = ImGui::GetCursorPos();
						UI::ShiftCursor(4.0f, 4.0f);
						ImGui::TextUnformatted(node->Name.c_str());
						ImGui::SetCursorPos(pos);
					}

					ImGui::InputText("##edit", &node->Name, inputTextFlags);
					DrawItemActivityOutline(UI::OutlineFlags_NoOutlineInactive);
				}
				//ImGui::PopTextWrapPos();
				ImGui::PopItemWidth();

				wasActive = DeactivateInputIfDragging(wasActive);
			}
			ImGui::EndHorizontal();
			ed::Group(node->Size);
			ImGui::EndVertical();
		}
		ed::EndNode();
		ImGui::PopStyleColor(); // Border
		ed::PopStyleColor(2); // StyleColor_NodeBg, StyleColor_NodeBorder

		// Popup hint when zoomed out
		if (ed::BeginGroupHint(ed::NodeId(node->ID)))
		{
			auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

			auto min = ed::GetGroupMin();

			ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
			ImGui::BeginGroup();
			ImGui::TextUnformatted(node->Name.c_str());
			ImGui::EndGroup();

			auto drawList = ed::GetHintBackgroundDrawList();

			auto hintBounds = UI::GetItemRect();
			auto hintFrameBounds = UI::RectExpanded(hintBounds, 8, 4);

			drawList->AddRectFilled(
				hintFrameBounds.GetTL(),
				hintFrameBounds.GetBR(),
				IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 1.0f);

			drawList->AddRect(
				hintFrameBounds.GetTL(),
				hintFrameBounds.GetBR(),
				IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 1.0f);
		}
		ed::EndGroupHint();
	}


	void NodeGraphEditorBase::DrawPinIcon(const Pin* pin, bool connected, int alpha)
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

			if (pin->GetType() == EPinType::Flow)
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


	void NodeGraphEditorBase::CheckContextMenus()
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
		else if (ed::ShowBackgroundContextMenu())
		{
			ImGui::OpenPopup("Create New Node");
			m_State->NewNodePopupOpening = true;
			m_State->NewNodeLinkPinId = 0;
		}
	}


	void NodeGraphEditorBase::DrawNodeContextMenu(Node* node)
	{
		ImGui::TextUnformatted("Node Context Menu");
		ImGui::Separator();
		if (node)
		{
			ImGui::Text("ID: %s", std::to_string(node->ID).c_str());
			ImGui::Text("Sort index: %d", node->SortIndex);
			ImGui::Text("Inputs: %d", (int)node->Inputs.size());
			ImGui::Text("Outputs: %d", (int)node->Outputs.size());
		}
		else
			ImGui::Text("Unknown node: %p", m_State->ContextNodeId.AsPointer());
		ImGui::Separator();
		if (ImGui::MenuItem("Delete"))
			ed::DeleteNode(m_State->ContextNodeId);
	}


	void NodeGraphEditorBase::DrawPinContextMenu(Pin* pin)
	{
		ImGui::TextUnformatted("Pin Context Menu");
		ImGui::Separator();
		if (pin)
		{
			ImGui::Text("ID: %s", std::to_string(pin->ID).c_str());
			if (pin->NodeID)
			{
				ImGui::Text("Node: %s", std::to_string(pin->NodeID).c_str());
			}
			else
				ImGui::Text("Node: %s", "<none>");

			ImGui::Text("Value type: %s", Utils::GetTypeName(pin->Value).c_str());
		}
		else
			ImGui::Text("Unknown pin: %s", m_State->ContextPinId.AsPointer());
	}


	void NodeGraphEditorBase::DrawLinkContextMenu(Link* link)
	{
		ImGui::TextUnformatted("Link Context Menu");
		ImGui::Separator();
		if (link)
		{
			ImGui::Text("ID: %s", std::to_string(link->ID).c_str());
			ImGui::Text("From: %s", std::to_string(link->StartPinID).c_str());
			ImGui::Text("To: %s", std::to_string(link->EndPinID).c_str());
		}
		else
			ImGui::Text("Unknown link: %p", m_State->ContextLinkId.AsPointer());
		ImGui::Separator();
		if (ImGui::MenuItem("Delete"))
			ed::DeleteLink(m_State->ContextLinkId);
	}


	void NodeGraphEditorBase::DrawBackgroundContextMenu(bool& isNewNodePoppedUp, ImVec2& newNodePostion)
	{
		isNewNodePoppedUp = true;
		newNodePostion = ed::ScreenToCanvas(ImGui::GetMousePosOnOpeningCurrentPopup());
		Node* node = nullptr;

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
		UI::Widgets::SearchWidget(searchString, "Search nodes...", &grabFocus);
		const bool searching = !searchString.empty();
		{
			UI::ScopedColourStack headerColours(
				ImGuiCol_Header, IM_COL32(255, 255, 255, 0),
				ImGuiCol_HeaderActive, IM_COL32(45, 46, 51, 255),
				ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 80)
			);

			//UI::ScopedColour scrollList(ImGuiCol_ChildBg, IM_COL32(255, 255, 255, 0));
			//UI::ScopedStyle scrollListStyle(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			UI::ShiftCursorY(GImGui->Style.WindowPadding.y + 2.0f);
			if (ImGui::BeginChild("##new_node_scroll_list", ImVec2(0.0f, 350.0f)))
			{
				// If subclass graph editor has any custom nodes,
				// the popup items for them can be added here
				if (onNodeListPopup)
					node = onNodeListPopup(searching, searchString);

				const auto& nodeRegistry = GetModel()->GetNodeTypes();
				for (const auto& [categoryName, category] : nodeRegistry)
				{
					// Can use this instead of the collapsing header
					//UI::PopupMenuHeader(categoryName, true, false);
					if (searching)
						ImGui::SetNextItemOpen(true);

					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(140, 140, 140, 255));
					if (UI::ContextMenuHeader(categoryName.c_str()))
					{
						ImGui::PopStyleColor(); // header Text
						//ImGui::Separator();

						ImGui::Indent();
						//-------------------------------------------------
						if (searching)
						{
							for (const auto& [nodeType, spawnFunction] : category)
							{
								if (UI::IsMatchingSearch(categoryName, searchString)
									|| UI::IsMatchingSearch(nodeType, searchString))
								{
									if (ImGui::MenuItem(nodeType.c_str()))
										node = GetModel()->CreateNode(categoryName, nodeType);
								}
							}
						}
						else
						{
							for (const auto& [nodeType, spawnFunction] : category)
							{
								if (ImGui::MenuItem(nodeType.c_str()))
									node = GetModel()->CreateNode(categoryName, nodeType);
							}
						}

						if (nodeRegistry.find(categoryName) != nodeRegistry.end())
							ImGui::Spacing();
						ImGui::Unindent();
					}
					else
					{
						ImGui::PopStyleColor(); // header Text
					}
				}
			}
			ImGui::EndChild();
		}

		if (node)
		{
			m_State->CreateNewNode = false;
			ed::SetNodePosition(ed::NodeId(node->ID), newNodePostion);
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


	void NodeGraphEditorBase::DrawDeferredComboBoxes(PinPropertyContext& pinContext)
	{
		ed::Suspend();
		{
			/// Asset pin search popup
			{
				if (pinContext.OpenAssetPopup)
					ImGui::OpenPopup("AssetSearch");

				// TODO: get item rect translated from canvas to screen coords and position popup properly

				// TODO: AssetSearchList should be combined with the button that opens it up.
				//		At the moment they are separate.
				bool clear = false;
				if (UI::Widgets::AssetSearchPopup("AssetSearch", NodeEditorDraw::s_SelectAssetContext.AssetType, NodeEditorDraw::s_SelectAssetContext.SelectedAssetHandle, &clear, "Search Asset", ImVec2{ 250.0f, 300.0f }))
				{
					if (auto pin = GetModel()->FindPin(NodeEditorDraw::s_SelectAssetContext.PinID))
					{
						pin->Value = Utils::CreateAssetHandleValue(NodeEditorDraw::s_SelectAssetContext.AssetType, clear? AssetHandle(0) : NodeEditorDraw::s_SelectAssetContext.SelectedAssetHandle);
						if (const auto& onPinValueChanged = GetModel()->onPinValueChanged)
						{
							onPinValueChanged(pin->NodeID, pin->ID);
						}
					}

					NodeEditorDraw::s_SelectAssetContext = {};
				}
			}

			/// Enum combo box
			{
				if (pinContext.OpenEnumPopup && GetModel()->FindPin(s_SelectedEnumContext.PinID))
					ImGui::OpenPopup("##EnumPopup");

				if (auto pin = GetModel()->FindPin(s_SelectedEnumContext.PinID))
				{
					bool changed = false;
					HZ_CORE_ASSERT(pin->EnumTokens.has_value() && pin->EnumTokens.value() != nullptr);
					const auto& tokens = **pin->EnumTokens;

					ImGui::SetNextWindowSize({ NodeEditorDraw::GetBestWidthForEnumCombo(tokens), 0.0f });
					if (UI::BeginPopup("##EnumPopup", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
					{
						for (int i = 0; i < tokens.size(); ++i)
						{
							ImGui::PushID(i);
							const bool itemSelected = (i == s_SelectedEnumContext.SelectedValue);
							if (ImGui::Selectable(tokens[i].name.data(), itemSelected))
							{
								changed = true;
								s_SelectedEnumContext.SelectedValue = i;
							}
							if (itemSelected)
								ImGui::SetItemDefaultFocus();
							ImGui::PopID();
						}
						UI::EndPopup();
					}

					if (changed)
					{
						pin->Value = s_SelectedEnumContext.ConstructValue(s_SelectedEnumContext.SelectedValue);
						if (GetModel()->onPinValueChanged)
							GetModel()->onPinValueChanged(pin->NodeID, pin->ID);
					}
				}
			}
		}
		ed::Resume();
	}


	//=============================================================================================
	/// IO Node Graph Editor
	//=============================================================================================


	IONodeGraphEditor::SelectedItem::operator bool() const
	{
		return (!Name.empty() || ID) && SelectionType != ESelectionType::Invalid;
	}

	bool IONodeGraphEditor::SelectedItem::IsSelected(EPropertyType type, std::string_view name) const
	{
		return SelectionType == FromPropertyType(type) && Name == name;
	}

	void IONodeGraphEditor::SelectedItem::Select(EPropertyType type, std::string_view name)
	{
		SelectionType = FromPropertyType(type);
		Name = name;
		ID = 0;
		ed::ClearSelection();
	}

	void IONodeGraphEditor::SelectedItem::SelectNode(UUID id)
	{
		SelectionType = ESelectionType::Node;
		ID = id;
		Name.clear();
	}

	void IONodeGraphEditor::SelectedItem::Clear()
	{
		SelectionType = ESelectionType::Invalid; Name.clear(); ID = 0;
	}

	EPropertyType IONodeGraphEditor::SelectedItem::GetPropertyType() const
	{
		switch (SelectionType)
		{
			case ESelectionType::Input: return EPropertyType::Input;
			case ESelectionType::Output: return EPropertyType::Output;
			case ESelectionType::LocalVariable: return EPropertyType::LocalVariable;
			case ESelectionType::Node:
			case ESelectionType::Invalid:
			default: return EPropertyType::Invalid;
		}
	}

	IONodeGraphEditor::ESelectionType IONodeGraphEditor::SelectedItem::FromPropertyType(EPropertyType type) const
	{
		switch (type)
		{
			case EPropertyType::Input: return ESelectionType::Input;
			case EPropertyType::Output: return ESelectionType::Output;
			case EPropertyType::LocalVariable: return ESelectionType::LocalVariable;
			case EPropertyType::Invalid:
			default: return ESelectionType::Invalid;
		}
	}


	IONodeGraphEditor::IONodeGraphEditor(const char* id) : NodeGraphEditorBase(id)
	{
		m_SelectedItem = CreateScope<SelectedItem>();

		m_ToolbarName = "Toolbar##" + m_Id;
		m_GraphInputsName = "Graph Inputs##" + m_Id;
		m_GraphDetailsName = "Graph Details##" + m_Id;
		m_DetailsName = "Details##" + m_Id;

		onNodeListPopup = [&](bool searching, std::string_view searchedString)
		{
			Node* newNode = nullptr;

			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(170, 170, 170, 255));
			if (ImGui::CollapsingHeader("Graph Inputs", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PopStyleColor(); // header Text
				ImGui::Separator();

				ImGui::Indent();
				//-------------------------------------------------
				if (searching)
				{
					for (const auto& graphInput : m_Model->GetInputs().GetNames())
					{
						if (UI::IsMatchingSearch("Graph Inputs", searchedString)
							|| UI::IsMatchingSearch(graphInput, searchedString))
						{
							if (ImGui::MenuItem(choc::text::replace(graphInput, "_", " ").c_str()))        // TODO: SpawnGraphOutputNode
								newNode = m_Model->SpawnGraphInputNode(graphInput);
						}
					}
				}
				else
				{
					for (const auto& graphInput : m_Model->GetInputs().GetNames())
					{
						if (ImGui::MenuItem(choc::text::replace(graphInput, "_", " ").c_str()))            // TODO: SpawnGraphOutputNode
							newNode = m_Model->SpawnGraphInputNode(graphInput);
					}
				}

				ImGui::Unindent();
			}
			else
			{
				ImGui::PopStyleColor(); // header Text
			}

			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(140, 140, 140, 255));
			if (UI::ContextMenuHeader("Local Variables", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PopStyleColor(); // header Text

				ImGui::Indent();
				//-------------------------------------------------
				if (searching)
				{
					for (const auto& localVariable : m_Model->GetLocalVariables().GetNames())
					{
						const std::string setter = localVariable + " Set";
						const std::string getter = localVariable + " Get";

						if (UI::IsMatchingSearch("Local Variables", searchedString)
							|| UI::IsMatchingSearch(setter, searchedString)
							|| UI::IsMatchingSearch(getter, searchedString))
						{
							if (ImGui::MenuItem(choc::text::replace(setter, "_", " ").c_str()))
								newNode = m_Model->SpawnLocalVariableNode(localVariable, false);

							if (ImGui::MenuItem(choc::text::replace(getter, "_", " ").c_str()))
								newNode = m_Model->SpawnLocalVariableNode(localVariable, true);
						}
					}
				}
				else
				{
					for (const auto& localVariable : m_Model->GetLocalVariables().GetNames())
					{
						const std::string setter = localVariable + " Set";
						const std::string getter = localVariable + " Get";

						if (ImGui::MenuItem(choc::text::replace(setter, "_", " ").c_str()))
							newNode = m_Model->SpawnLocalVariableNode(localVariable, false);

						if (ImGui::MenuItem(choc::text::replace(getter, "_", " ").c_str()))
							newNode = m_Model->SpawnLocalVariableNode(localVariable, true);
					}
				}

				ImGui::Unindent();
			}
			else
			{
				ImGui::PopStyleColor(); // header Text
			}

			return newNode;
		};
	}


	IONodeGraphEditor::~IONodeGraphEditor() = default;


	void IONodeGraphEditor::OnRender()
	{
		ImGui::SetNextWindowClass(GetWindowClass());

		if (ImGui::Begin(m_ToolbarName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse))
		{
			//EnsureWindowIsDocked(ImGui::GetCurrentWindow());
			DrawToolbar();
		}
		ImGui::End(); // Toolbar

		ImGui::SetNextWindowClass(GetWindowClass());
		if (ImGui::Begin(m_GraphDetailsName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse))
		{
			//EnsureWindowIsDocked(ImGui::GetCurrentWindow());
			DrawDetailsPanel();
		}
		ImGui::End(); // Graph details

		ImGui::SetNextWindowClass(GetWindowClass());
		if (ImGui::Begin(m_GraphInputsName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollWithMouse))
		{
			//EnsureWindowIsDocked(ImGui::GetCurrentWindow());
			DrawGraphIO();

			if (ed::HasSelectionChanged())
			{
				std::vector<UUID> selectedNodes = GetSelectedNodeIDs();
				if (selectedNodes.size() == 1)
				{
					m_SelectedItem->SelectNode(selectedNodes.back());
				}
				else if (m_SelectedItem->SelectionType == ESelectionType::Node)
				{
					m_SelectedItem->Clear();
				}
			}

			ImGui::SetNextWindowClass(GetWindowClass());
			if (ImGui::Begin(m_DetailsName.c_str()))
			{
				DrawSelectedDetails();
			}
			ImGui::End(); // Selected item details
		}
		ImGui::End(); // Graph Inputs
	}


	ImGuiWindowFlags IONodeGraphEditor::GetWindowFlags()
	{
		return ImGuiWindowFlags_NoCollapse;
	}


	void IONodeGraphEditor::OnWindowStylePush()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

		/*if (ImGui::Begin("Style Editor"))
			ImGui::ShowStyleEditor();

		ImGui::End();*/
	}


	void IONodeGraphEditor::OnWindowStylePop()
	{
		ImGui::PopStyleVar(2); // ImGuiStyleVar_WindowPadding, ImGuiStyleVar_FrameBorderSize
	}


	const char* IONodeGraphEditor::GetIconForSimpleNode(const std::string& simpleNodeIdentifier) const
	{
		if (choc::text::contains(simpleNodeIdentifier, "Add")) return "+";
		if (choc::text::contains(simpleNodeIdentifier, "Subtract")) return "-";
		if (choc::text::contains(simpleNodeIdentifier, "Mult")) return "x";
		if (choc::text::contains(simpleNodeIdentifier, "Divide")) return "/";
		if (choc::text::contains(simpleNodeIdentifier, "Modulo")) return "%";
		if (choc::text::contains(simpleNodeIdentifier, "Pow")) return "^";
		if (choc::text::contains(simpleNodeIdentifier, "Log")) return "log";
		if (choc::text::contains(simpleNodeIdentifier, "Less Equal")) return "<=";
		if (choc::text::contains(simpleNodeIdentifier, "Not Equal")) return "!=";
		if (choc::text::contains(simpleNodeIdentifier, "Greater Equal")) return ">=";
		if (choc::text::contains(simpleNodeIdentifier, "Equal")) return "==";
		if (choc::text::contains(simpleNodeIdentifier, "Less")) return "<";
		if (choc::text::contains(simpleNodeIdentifier, "Greater")) return ">";

		else return nullptr;
	}


	void IONodeGraphEditor::SelectNode(UUID id)
	{
		m_SelectedItem->SelectNode(id);
	}


	bool IONodeGraphEditor::IsAnyNodeSelected() const
	{
		return m_SelectedItem->SelectionType == ESelectionType::Node;
	}


	void IONodeGraphEditor::ClearSelection()
	{
		m_SelectedItem->Clear();
	}


	void IONodeGraphEditor::DrawToolbar()
	{
		auto* drawList = ImGui::GetWindowDrawList();

		const float spacing = 16.0f;

		ImGui::BeginHorizontal("ToolbarHorizontalLayout", ImGui::GetContentRegionAvail());
		ImGui::Spring(0.0f, spacing);

		// Compile Button
		{
			float compileIconWidth = EditorResources::CompileIcon->GetWidth() * 0.9f;
			float compileIconHeight = EditorResources::CompileIcon->GetWidth() * 0.9f;
			if (ImGui::InvisibleButton("compile", ImVec2(compileIconWidth, compileIconHeight)))
			{
				m_Model->CompileGraph();
			}
			UI::DrawButtonImage(EditorResources::CompileIcon, IM_COL32(235, 165, 36, 200),
								IM_COL32(235, 165, 36, 255),
								IM_COL32(235, 165, 36, 120));

			UI::SetTooltip("Compile");

			if (m_Model->IsPlayerDirty())
			{
				UI::ShiftCursorX(-6.0f);
				ImGui::Text("*");
			}
		}

		// Save Button
		{
			const float saveIconWidth = static_cast<float>(EditorResources::SaveIcon->GetWidth());
			const float saveIconHeight = static_cast<float>(EditorResources::SaveIcon->GetWidth());

			if (ImGui::InvisibleButton("saveGraph", ImVec2(saveIconWidth, saveIconHeight)))
			{
				m_Model->SaveAll();
			}
			const int iconOffset = 4;
			auto iconRect = UI::GetItemRect();
			iconRect.Min.y += iconOffset;
			iconRect.Max.y += iconOffset;

			UI::DrawButtonImage(EditorResources::SaveIcon, IM_COL32(102, 204, 163, 200),
								IM_COL32(102, 204, 163, 255),
								IM_COL32(102, 204, 163, 120), iconRect);

			UI::SetTooltip("Save graph");
		}

		// Viewport to content
		{
			if (ImGui::Button("Navigate to Content"))
				ed::NavigateToContent();
		}

		const float leftSizeOffset = ImGui::GetCursorPosX();

		ImGui::Spring();

		// Playback buttons
		// ---------------

		// Play Button
		{
			if (ImGui::InvisibleButton("PlayGraphButton", ImVec2(ImGui::GetTextLineHeightWithSpacing() + 4.0f,
				ImGui::GetTextLineHeightWithSpacing() + 4.0f)))
			{
				if (m_Model->IsPlayerDirty())
					m_Model->CompileGraph();

				if (!m_Model->IsPlayerDirty() && m_Model->onPlay)
					m_Model->onPlay();
			}

			UI::DrawButtonImage(EditorResources::PlayIcon, IM_COL32(102, 204, 163, 200),
								IM_COL32(102, 204, 163, 255),
								IM_COL32(102, 204, 163, 120),
								UI::RectExpanded(UI::GetItemRect(), 1.0f, 1.0f));
		}

		// Stop Button
		{
			if (ImGui::InvisibleButton("StopGraphButton", ImVec2(ImGui::GetTextLineHeightWithSpacing() + 4.0f,
				ImGui::GetTextLineHeightWithSpacing() + 4.0f)))
			{
				if (m_Model->onStop) m_Model->onStop(false);
			}

			UI::DrawButtonImage(EditorResources::StopIcon, IM_COL32(102, 204, 163, 200),
								IM_COL32(102, 204, 163, 255),
								IM_COL32(102, 204, 163, 120),
								UI::RectExpanded(UI::GetItemRect(), -1.0f, -1.0f));
		}

		ImGui::Spring(1.0f, leftSizeOffset);

		ImGui::Spring(0.0f, spacing);
		ImGui::EndHorizontal();
	}


	void IONodeGraphEditor::DrawDetailsPanel()
	{
		auto& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		std::vector<ed::NodeId> selectedNodes;
		std::vector<ed::LinkId> selectedLinks;
		selectedNodes.resize(ed::GetSelectedObjectCount());
		selectedLinks.resize(ed::GetSelectedObjectCount());

		int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
		int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

		selectedNodes.resize(nodeCount);
		selectedLinks.resize(linkCount);

		UI::ShiftCursorY(12.0f);
		{
			ImGui::CollapsingHeader("##NODES", ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Leaf);
			ImGui::SetItemAllowOverlap();
			ImGui::SameLine();
			UI::ShiftCursorX(-8.0f);
			ImGui::Text("NODES");
		}

		if (ImGui::BeginListBox("##nodesListBox", ImVec2(-FLT_MIN, -FLT_MIN)))
		{
			for (auto& node : m_Model->GetNodes())
			{
				UI::ScopedID nodeID((int)node->ID);
				auto start = ImGui::GetCursorScreenPos();

				bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), ed::NodeId(node->ID)) != selectedNodes.end();
				if (ImGui::Selectable((node->Name + "##" + std::to_string(node->ID)).c_str(), &isSelected))
				{
					if (io.KeyCtrl)
					{
						if (isSelected)
							ed::SelectNode(ed::NodeId(node->ID), true);
						else
							ed::DeselectNode(ed::NodeId(node->ID));
					}
					else
						ed::SelectNode(ed::NodeId(node->ID), false);

					ed::NavigateToSelection();
				}
				if (UI::IsItemHovered() && !node->State.empty())
					ImGui::SetTooltip("State: %s", node->State.c_str());

				//? Display node IDs
				/*auto id = std::string("(") + std::to_string(node.ID) + ")";
				auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);

				ImGui::SameLine();
				ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - textSize.x);
				ImGui::Text(id.c_str());*/
			}

			ImGui::EndListBox();
		}
	}


	void IONodeGraphEditor::DrawNodeDetails(UUID nodeID)
	{
		auto node = m_Model->FindNode(nodeID);
		if (!node)
			return;

		auto propertyType = m_Model->GetPropertyTypeOfNode(node);
		if (propertyType != EPropertyType::Invalid)
		{
			std::string_view propertyName;
			switch (propertyType)
			{
				case EPropertyType::Input: propertyName = node->Outputs[0]->Name; break;
				case EPropertyType::Output: propertyName = node->Inputs[0]->Name; break;
				case EPropertyType::LocalVariable: if(node->Inputs.size() > 0) propertyName = node->Inputs[0]->Name; else propertyName = node->Outputs[0]->Name; break;
				default: propertyName = node->Name; break;
			}
			DrawPropertyDetails(propertyType, propertyName);
			return;
		}

		//! For now we only draw details for a Comment node
		if (node->Type != NodeType::Comment)
			return;

		if (UI::PropertyGridHeader("Comment"))
		{
			UI::BeginPropertyGrid();

			// Comment colour
			{
				glm::vec4 colour;
				memcpy(&colour.x, &node->Color.Value.x, sizeof(float) * 4);

				if (UI::PropertyColor("Colour", colour))
					memcpy(&node->Color.Value.x, &colour.x, sizeof(float) * 4);
			}

			UI::EndPropertyGrid();
			ImGui::TreePop();
		}
	}


	// Note(0x): c++20 would allow this as a lambda inside DrawGraphIO()
	template<typename T, AssetType assetType>
	bool EditAssetField(const std::string& id, choc::value::ValueView& valueView, bool changed)
	{
		ImGui::PushID((id + "AssetReference").c_str());

		AssetHandle selected = 0;
		Ref<T> asset;

		selected = static_cast<AssetHandle>(Utils::GetAssetHandleFromValue(valueView));

		if (AssetManager::IsAssetHandleValid(selected))
		{
			asset = AssetManager::GetAsset<T>(selected);
		}
		// Initialize choc::Value class object with correct asset type using helper function
		bool assetDropped = false;
		if (UI::AssetReferenceDropTargetButton(id.c_str(), asset, assetType, assetDropped))
		{
			ImGui::OpenPopup((id + "AssetPopup").c_str());
		}

		if (assetDropped)
		{
			Utils::SetAssetHandleValue(valueView, asset->Handle);
			changed = true;
		}

		bool clear = false;
		if (UI::Widgets::AssetSearchPopup((id + "AssetPopup").c_str(), assetType, selected, &clear))
		{
			if (clear)
			{
				selected = 0;
			}
			Utils::SetAssetHandleValue(valueView, selected);
			changed = true;
		}

		ImGui::PopID();
		return changed;
	}


	void IONodeGraphEditor::DrawPropertyDetails(EPropertyType graphPropertyType, std::string_view propertyName)
	{
		EPropertyType propertyType = (EPropertyType)graphPropertyType;

		const float panelWidth = ImGui::GetContentRegionAvail().x;

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Indent();

		std::string oldName(propertyName);
		std::string newName(propertyName);

		choc::value::Value value = m_Model->GetPropertySet(propertyType).GetValue(newName);

		// Name
		{
			static char buffer[128]{};
			memset(buffer, 0, sizeof(buffer));
			memcpy(buffer, newName.data(), std::min(newName.size(), sizeof(buffer)));

			const auto inputTextFlags = ImGuiInputTextFlags_AutoSelectAll
				| ImGuiInputTextFlags_EnterReturnsTrue
				| ImGuiInputTextFlags_CallbackAlways;

			ImGui::Text("Input Name");
			ImGui::SameLine();
			UI::ShiftCursorY(-3.0f);

			// TODO: move this into some sort of utility header
			auto validateName = [](ImGuiInputTextCallbackData* data) -> int
			{
				const auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
				const auto isSafeIdentifierChar = [&isDigit](char c)
				{
					return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == ' ' || isDigit(c);
				};

				const int lastCharPos = glm::max(data->CursorPos - 1, 0);
				char lastChar = data->Buf[lastCharPos];

				if (data->BufTextLen >= 1)
				{
					// Must not start with '_' or ' '
					if (data->Buf[0] == '_' || data->Buf[0] == ' ' || isDigit(data->Buf[0]) || !isSafeIdentifierChar(data->Buf[0]))
						data->DeleteChars(0, 1);
				}

				if (data->BufTextLen < 3)
					return 0;

				if (lastChar == ' ')
				{
					// Block double whitespaces
					std::string_view str(data->Buf);
					size_t doubleSpacePos = str.find("  ");
					if (doubleSpacePos != std::string::npos)
						data->DeleteChars((int)doubleSpacePos, 1);

					return 1;
				}
				else if (!isSafeIdentifierChar(lastChar))
				{
					data->DeleteChars(lastCharPos, 1);
				}

				return 0;
			};

			UI::InputText("##inputName", buffer, 127, inputTextFlags, validateName);

			if (ImGui::IsItemDeactivatedAfterEdit() && buffer[0])
			{
				if (m_Model->IsGraphPropertyNameValid(propertyType, buffer))
				{
					// Rename item in the list
					newName = buffer;
					newName = choc::text::trim(newName);
					m_Model->RenameProperty(propertyType, oldName, newName);

					m_SelectedItem->Name = buffer;
				}
				else
				{
					// TODO: show some informative warning
				}
			}
		}

		// Type & Value
		{
			const auto& types = m_Model->GetInputTypes();

			choc::value::Type type = value.getType();
			choc::value::Type arrayType;
			bool isArrayOrVector = false;
			if (value.isArray())
			{
				arrayType = type;
				isArrayOrVector = true;
				type = type.getElementType();
			}

			// find index of type in types
			int32_t selected = -1;
			auto typeName = Utils::GetTypeName(value);
			for (int i = 0; i < types.size(); i++)
			{
				if (types[i] == typeName)
				{
					selected = i;
					break;
				}
			}
			if (selected == -1)
			{
				HZ_CORE_ERROR("Invalid type of Graph Input!");
				selected = 0;
			}

			auto createValueFromSelectedIndex = [&types](int32_t index)
			{
				const auto& type = Utils::GetTypeFromName(types[index]);
				auto value = choc::value::Value(type);
				return value;
			};

			// Change the type the value, or the type of array element
			ImGui::SetCursorPosX(0.0f);

			const float checkboxWidth = 84.0f;
			ImGui::BeginChildFrame(ImGui::GetID("TypeRegion"), ImVec2(ImGui::GetContentRegionAvail().x - checkboxWidth, ImGui::GetFrameHeight() * 1.4f), ImGuiWindowFlags_NoBackground);
			ImGui::Columns(2);

			auto getColourForType = [&](int optionNumber)
			{
				return GetModel()->GetValueColor(createValueFromSelectedIndex(optionNumber));
			};

			if (UI::PropertyDropdown("Type", types, (int32_t)types.size(), &selected, getColourForType))
			{
				choc::value::Value newValue = createValueFromSelectedIndex(selected);

				if (isArrayOrVector)
				{
					choc::value::Value arrayValue = choc::value::createArray(value.size(), [&](uint32_t i) { return choc::value::Value(newValue.getType()); });

					m_Model->ChangePropertyType(propertyType, newName, arrayValue);
				}
				else
				{
					m_Model->ChangePropertyType(propertyType, newName, newValue);
				}
			}
			ImGui::EndColumns();
			ImGui::EndChildFrame();

			// Update value handle in case type or name has changed
			value = m_Model->GetPropertySet(propertyType).GetValue(newName);
			isArrayOrVector = value.isArray();

			// Switch between Array vs Value

			ImGui::SameLine(0.0f, 0.0f);
			UI::ShiftCursorY(3);

			if (value.isVoid())
				ImGui::BeginDisabled();

			bool isArray = value.isArray();
			if (UI::Checkbox("##IsArray", &isArray))
			{
				if (isArray) // Became array
				{
					choc::value::Value arrayValue = choc::value::createEmptyArray();
					arrayValue.addArrayElement(value);

					m_Model->ChangePropertyType(propertyType, newName, arrayValue);
					isArrayOrVector = true;
					value = arrayValue;
				}
				else // Became simple
				{
					auto newValue = choc::value::Value(value.getType().getElementType());

					m_Model->ChangePropertyType(propertyType, newName, newValue);
					isArrayOrVector = false;
					value = newValue;
				}
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("Is Array");
			
			if (value.isVoid())
				ImGui::EndDisabled();

			ImGui::Unindent();

			ImGui::Dummy(ImVec2(panelWidth, 10.0f));

			// Assign default value to the Value or Array elements

			auto valueEditField = [](const char* label, choc::value::ValueView& valueView, bool* removeButton = nullptr)
			{
				ImGui::Text(label);
				ImGui::NextColumn();
				ImGui::PushItemWidth(-ImGui::GetFrameHeight());

				bool changed = false;
				const std::string id = "##" + std::string(label);

				if (valueView.isFloat())
				{
					float v = valueView.get<float>();
					if (UI::DragFloat(id.c_str(), &v, 0.01f, 0.0f, 0.0f, "%.2f"))
					{
						valueView.set(v);
						changed = true;
					}

				}
				else if (valueView.isInt32())
				{
					int32_t v = valueView.get<int32_t>();
					if (UI::DragInt32(id.c_str(), &v, 0.1f, 0, 0))
					{
						valueView.set(v);
						changed = true;
					}
				}
				else if (valueView.isBool())
				{
					bool v = valueView.get<bool>();
					if (UI::Checkbox(id.c_str(), &v))
					{
						valueView.set(v);
						changed = true;
					}
				}
				else if (valueView.isString())
				{
					std::string v = valueView.get<std::string>();

					const auto inputTextFlags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue;
					UI::InputText(id.c_str(), &v, inputTextFlags);
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						valueView.set(v);
						changed = true;
					}
				}
				else if (Utils::IsAssetHandle<AssetType::Audio>(valueView))     changed = EditAssetField<AudioFile, AssetType::Audio>(id, valueView, changed);
				else if (Utils::IsAssetHandle<AssetType::Animation>(valueView)) changed = EditAssetField<AnimationAsset, AssetType::Animation>(id, valueView, changed);
				else if (Utils::IsAssetHandle<AssetType::Skeleton>(valueView))  changed = EditAssetField<SkeletonAsset, AssetType::Skeleton>(id, valueView, changed);
				else if (valueView.isObjectWithClassName(type::type_name<glm::vec3>()))
				{
					auto v = static_cast<glm::vec3*>(valueView.getRawData());
					bool manuallyEdited = false;
					changed = UI::Widgets::EditVec3(id.c_str(), ImVec2(ImGui::GetContentRegionAvail().x  - ImGui::GetFrameHeight() - 8.0f, ImGui::GetFrameHeightWithSpacing()), 0.0f, manuallyEdited, *v);
				}
				else
				{
					HZ_CORE_ERROR("Invalid type of Graph Input!");
				}

				ImGui::PopItemWidth();

				if (removeButton != nullptr)
				{
					ImGui::SameLine();

					ImGui::PushID((id + "removeButton").c_str());

					if (ImGui::SmallButton("x"))
						*removeButton = true;

					ImGui::PopID();
				}

				ImGui::NextColumn();

				return changed;
			};


			ImGui::GetWindowDrawList()->AddRectFilled(
				ImGui::GetCursorScreenPos() - ImVec2(0.0f, 2.0f),
				ImGui::GetCursorScreenPos() + ImVec2(panelWidth, ImGui::GetTextLineHeightWithSpacing()),
				ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), 2.5f);

			ImGui::Spacing(); ImGui::SameLine(); UI::ShiftCursorY(2.0f);
			ImGui::Text("Default Value");

			ImGui::Spacing();
			ImGui::Dummy(ImVec2(panelWidth, 4.0f));

			ImGui::Indent();

			const bool isTriggerType = value.isVoid();

			if (!isTriggerType && !isArrayOrVector)
			{
				ImGui::Columns(2);
				if (valueEditField("Value", value.getViewReference()))
				{
					m_Model->SetPropertyValue(propertyType, newName, value);
				}
				ImGui::EndColumns();
			}
			else if (!isTriggerType) // Array
			{
				ImGui::BeginHorizontal("ArrayElementsInfo", { ImGui::GetContentRegionAvail().x, 0.0f });
				uint32_t size = value.size();
				ImGui::Text((std::to_string(size) + " Array elements").c_str());

				ImGui::Spring();

				//? For now limiting max number of elements in array to fit into
				//? choc's small vector optimization and to prevent large amount
				//? of data being copied to SoundGraph player
				const bool reachedMaxArraySize = value.size() >= 32;
				if (reachedMaxArraySize)
				{
					ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
					ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
				}

				bool changed = false;
				bool invalidatePlayer = false;

				if (ImGui::SmallButton("Add Element"))
				{
					value.addArrayElement(choc::value::Value(value.getType().getElementType()));

					changed = true;
					invalidatePlayer = true;
				}

				if (reachedMaxArraySize)
				{
					ImGui::PopItemFlag(); // ImGuiItemFlags_Disabled
					ImGui::PopStyleColor(); // ImGuiCol_Text
				}

				ImGui::Spring(0.0f, 4.0f);
				ImGui::EndHorizontal();

				ImGui::Spacing();

				ImGui::Columns(2);

				int indexToRemove = -1;
				UI::PushID();
				for (uint32_t i = 0; i < value.size(); ++i)
				{
					choc::value::ValueView vv = value[i];

					bool removeThis = false;
					if (valueEditField(("[ " + std::to_string(i) + " ]").c_str(), vv, &removeThis))
						changed = true;

					if (value.size() > 1 && removeThis) indexToRemove = i;
				}
				UI::PopID();

				if (indexToRemove >= 0)
				{
					bool skipIteration = false;
					value = choc::value::createArray(value.size() - 1, [&value, &skipIteration, indexToRemove](uint32_t i)
					{
						if (!skipIteration)
							skipIteration = i == indexToRemove;
						return skipIteration ? value[i + 1] : value[i];
					});

					changed = true;
					invalidatePlayer = true;
				}

				if (invalidatePlayer)
					m_Model->InvalidatePlayer();

				if (changed)
				{
					m_Model->SetPropertyValue(propertyType, newName, value);
				}

				ImGui::EndColumns();
			}
			// TODO: invalidate connections that were using this input

			ImGui::Unindent();
		}
	}


	void IONodeGraphEditor::DrawGraphIO()
	{
		const float panelWidth = ImGui::GetContentRegionAvail().x;

		auto addInputButton = [&]()
		{
			ImGui::SameLine(panelWidth - ImGui::GetFrameHeight() - ImGui::CalcTextSize("Add Input").x);
			if (ImGui::SmallButton("Add Input"))
			{
				std::string name = m_Model->AddPropertyToGraph(EPropertyType::Input, choc::value::createFloat32(0.0f));
				if(!name.empty())
					m_SelectedItem->Select(EPropertyType::Input, name);
			}
		};

		const int leafFlags = ImGuiTreeNodeFlags_Leaf
			| ImGuiTreeNodeFlags_NoTreePushOnOpen
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_AllowItemOverlap;

		const int headerFlags = ImGuiTreeNodeFlags_CollapsingHeader
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_DefaultOpen;

		// INPUTS list
		ImGui::Spacing();
		if (UI::PropertyGridHeader("INPUTS"))
		{
			addInputButton();

			// For now, don't show "Input Action" node.
			// There are no "details" for these.
			//ImGui::TreeNodeEx("Input Action", leafFlags);

			std::string inputToRemove;

			for (auto& input : m_Model->GetInputs().GetNames())
			{
				bool selected = m_SelectedItem->IsSelected(EPropertyType::Input, input);
				int flags = selected ? leafFlags | ImGuiTreeNodeFlags_Selected : leafFlags;

				// Colouring text, just because
				ImColor textColour = m_Model->GetValueColor(m_Model->GetInputs().GetValue(input));

				ImGui::PushStyleColor(ImGuiCol_Text, textColour.Value);
				ImGui::TreeNodeEx(input.c_str(), flags);
				ImGui::PopStyleColor();

				if (ImGui::BeginDragDropSource())
				{
					ImGui::TextUnformatted(input.c_str());
					ImGui::SetDragDropPayload("input_payload", input.c_str(), input.size() + 1);
					ImGui::EndDragDropSource();
				}

				if (ImGui::IsItemClicked())
					m_SelectedItem->Select(EPropertyType::Input, input);

				// Remove button
				ImGui::SameLine();
				ImGui::SetCursorPosX(panelWidth - ImGui::GetFrameHeight());

				ImGui::PushID((input + "removeInput").c_str());
				if (ImGui::SmallButton("x"))
					inputToRemove = input;
				ImGui::PopID();
			}

			if (!inputToRemove.empty())
			{
				if (inputToRemove == m_SelectedItem->Name)
					m_SelectedItem->Clear();

				m_Model->RemovePropertyFromGraph(EPropertyType::Input, inputToRemove);
			}

			ImGui::TreePop();
		}
		else
		{
			addInputButton();
		}

		// For now, do not show Outputs list, because:
		//    a) There are no details to display for selected output
		//    b) More than one output is not supported
		//    c) Adding output is not supported
		//auto addOutputButton = [&]
		//{
		//	ImGui::SameLine(panelWidth - ImGui::GetFrameHeight() - ImGui::CalcTextSize("Add Output").x);
		//	if (ImGui::SmallButton("Add Output"))
		//	{
		//		// TODO: add output
		//		HZ_CORE_WARN("Add Output to Graph not implemented.");
		//	}
		//};
		//
		//// OUTPUTS list
		//if (UI::PropertyGridHeader("OUTPUTS"))
		//{
		//	addOutputButton();
		//
		// Note: graph isnt necessarily Audio, so "Output Audio" is not necessarily
		// what the output is
		//	ImGui::TreeNodeEx("Output Audio", leafFlags);
		//
		//	ImGui::TreePop();
		//}
		//else
		//{
		//	addOutputButton();
		//}

		auto addVariableButton = [&]()
		{
			ImGui::SameLine(panelWidth - ImGui::GetFrameHeight() - ImGui::CalcTextSize("Add Variable").x);
			if (ImGui::SmallButton("Add Variable"))
			{
				std::string name = m_Model->AddPropertyToGraph(EPropertyType::LocalVariable, choc::value::createFloat32(0.0f));
				if (!name.empty())
					m_SelectedItem->Select(EPropertyType::LocalVariable, name);
			}
		};

		// Variables / Reroutes
		if (UI::PropertyGridHeader("LOCAL VARIABLES"))
		{
			addVariableButton();

		std::string variableToRemove;
		for (auto& input : m_Model->GetLocalVariables().GetNames())
		{
			bool selected = m_SelectedItem->IsSelected(EPropertyType::LocalVariable, input);
			int flags = selected ? leafFlags | ImGuiTreeNodeFlags_Selected : leafFlags;

		// Colouring text, just because
		ImColor textColor = m_Model->GetValueColor(m_Model->GetLocalVariables().GetValue(input));

		ImGui::PushStyleColor(ImGuiCol_Text, textColor.Value);
		ImGui::TreeNodeEx(input.c_str(), flags);
		ImGui::PopStyleColor();

		if (ImGui::BeginDragDropSource())
		{
			ImGui::TextUnformatted(input.c_str());
			ImGui::SameLine();
			ImGui::TextUnformatted((ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift)) ? "(set)" : "(get)");
			ImGui::SetDragDropPayload("localvar_payload", input.c_str(), input.size() + 1);
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsItemClicked())
			m_SelectedItem->Select(EPropertyType::LocalVariable, input);

		// Remove button
		ImGui::SameLine();
		ImGui::SetCursorPosX(panelWidth - ImGui::GetFrameHeight());

			ImGui::PushID((input + "removeVariable").c_str());
			if (ImGui::SmallButton("x"))
				variableToRemove = input;
			ImGui::PopID();
		}

		if (!variableToRemove.empty())
		{
			if (variableToRemove == m_SelectedItem->Name)
				m_SelectedItem->Clear();

				m_Model->RemovePropertyFromGraph(EPropertyType::LocalVariable, variableToRemove);
			}
			ImGui::TreePop();
		}
		else
		{
			addVariableButton();
		}
	}


	void IONodeGraphEditor::DrawSelectedDetails()
	{
		const float panelWidth = ImGui::GetContentRegionAvail().x;
		
		if (*m_SelectedItem)
		{
			if (m_SelectedItem->SelectionType == ESelectionType::Node)
				DrawNodeDetails(m_SelectedItem->ID);
			else
				DrawPropertyDetails(m_SelectedItem->GetPropertyType(), m_SelectedItem->Name);
		}

		//? DBG Info
		ImGui::Dummy(ImVec2(panelWidth, 40.0f));
		UI::Checkbox("##IDs", &m_ShowNodeIDs);
		ImGui::SameLine();
		ImGui::TextUnformatted("Show IDs");
		ImGui::SameLine(0, 20.0f);
		UI::Checkbox("##Indices", &m_ShowSortIndices);
		ImGui::SameLine();
		ImGui::TextUnformatted("Show Sort Indices");
	}

} // namespace Hazel
