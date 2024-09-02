#include <hzpch.h>
#include "SoundGraphGraphEditor.h"
#include "SoundGraphNodeEditorModel.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Vendor/imgui-node-editor/widgets.h"

#include <choc/text/choc_StringUtilities.h>
#include <choc/text/choc_FloatToString.h>
#include <imgui-node-editor/builders.h>
#include <imgui-node-editor/imgui_node_editor.h>

namespace ed = ax::NodeEditor;

namespace Hazel {

	//=============================================================================================
	/// SoundGraph Node Graph Editor
	SoundGraphNodeGraphEditor::SoundGraphNodeGraphEditor() : IONodeGraphEditor("SoundGraph Sound")
	{
		onDragDropOntoCanvas = [&]() {
			auto model = static_cast<SoundGraphNodeEditorModel*>(GetModel());
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
		};
	}

	void SoundGraphNodeGraphEditor::SetAsset(const Ref<Asset>& asset)
	{
		SetModel(std::make_unique<SoundGraphNodeEditorModel>(asset.As<SoundGraphAsset>()));
		IONodeGraphEditor::SetAsset(asset);
	}


	void SoundGraphNodeGraphEditor::OnRenderOnCanvas(ImVec2 min, ImVec2 max)
	{
		// Draw current zoom level
		{
			UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);
			const ImVec2 zoomLabelPos({ min.x + 20.0f, max.y - 40.0f });
			const std::string zoomLabelText = "Zoom " + choc::text::floatToString(ed::GetCurrentZoom(), 1, true);

			ImGui::GetWindowDrawList()->AddText(zoomLabelPos, IM_COL32(255, 255, 255, 60), zoomLabelText.c_str());
		}

		// Draw current playback position
		uint64_t currentMs = static_cast<SoundGraphNodeEditorModel*>(GetModel())->GetCurrentPlaybackFrame() / 48;
		// Not an ideal check, but there's probably no other case where current
		// playback position is 0 except for when the player isn't playing.
		if (currentMs > 0)
		{
			UI::ScopedFont largeFont(ImGui::GetIO().Fonts->Fonts[1]);

			const ImVec2 timeCodePosition({ min.x + 20.0f, min.y + 10.0f });
			const std::string timecodeText = Utils::DurationToString(std::chrono::milliseconds(currentMs));

			ImGui::GetWindowDrawList()->AddText(timeCodePosition, IM_COL32(255, 255, 255, 60), timecodeText.c_str());
		}
	}


	bool SoundGraphNodeGraphEditor::DrawPinPropertyEdit(PinPropertyContext& context)
	{
		bool modified = false;

		Pin* pin = context.pin;

		switch ((SoundGraph::SGTypes::ESGPinType)pin->GetType())
		{
			case SoundGraph::SGTypes::ESGPinType::Bool:         modified = NodeEditorDraw::PropertyBool(pin->Value); break;
			case SoundGraph::SGTypes::ESGPinType::Int:          modified = NodeEditorDraw::PropertyInt(pin->Value); break;
			case SoundGraph::SGTypes::ESGPinType::Float:        modified = NodeEditorDraw::PropertyFloat(pin->Value); break;
			case SoundGraph::SGTypes::ESGPinType::String:       modified = NodeEditorDraw::PropertyString(pin->Value); break;
			case SoundGraph::SGTypes::ESGPinType::AudioAsset:   modified = NodeEditorDraw::PropertyAsset<AudioFile, AssetType::Audio>(pin->Value, pin, context.OpenAssetPopup); break;
			case SoundGraph::SGTypes::ESGPinType::Enum:
			{
				const int enumValue = pin->Value["Value"].get<int>();

				// TODO: JP. not ideal to do this callback style value assignment, should make it more generic.
				auto constructEnumValue = [](int selected) { return ValueFrom(SoundGraph::SGTypes::TSGEnum{ selected }); };

				modified = NodeEditorDraw::PropertyEnum(enumValue, pin, context.OpenEnumPopup, constructEnumValue);
			}
			break;
			default:

				break;
		}

		return modified;
	}


	void SoundGraphNodeGraphEditor::DrawPinIcon(const Pin* pin, bool connected, int alpha)
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

			if (pin->IsType(SoundGraph::SGTypes::ESGPinType::Audio))
			{
				image = connected || acceptingLink ? EditorResources::PinAudioConnectIcon : EditorResources::PinAudioDisconnectIcon;
			}
			/*else if (pin->IsType(SoundGraph::SGTypes::ESGPinType::Flow))
			{
				image = connected || acceptingLink ? EditorResources::PinFlowConnectIcon : EditorResources::PinFlowDisconnectIcon;
			}*/
			else
			{
				image = connected || acceptingLink ? EditorResources::PinValueConnectIcon : EditorResources::PinValueDisconnectIcon;
			}

			const auto iconWidth = image->GetWidth();
			const auto iconHeight = image->GetHeight();
			ax::Widgets::ImageIcon(ImVec2(static_cast<float>(iconWidth), static_cast<float>(iconHeight)), UI::GetTextureID(image), connected, (float)pinIconSize, color, ImColor(32, 32, 32, alpha));
		}
	}

	void SoundGraphNodeGraphEditor::OnUpdate(Timestep ts)
	{
		if (IsOpen())
			static_cast<SoundGraphNodeEditorModel*>(GetModel())->OnUpdate(ts);
	}

} // namespace Hazel
