#pragma once

#include "Nodes.h"
#include "NodeEditorModel.h"
#include "NodeGraphUtils.h"

#include "Engine/Editor/AssetEditorPanel.h"

namespace ax::NodeEditor {

	struct Style;
	struct EditorContext;

	namespace Utilities {
		struct BlueprintNodeBuilder; // TODO: rename it and rewrite it to use Hazel::Texture2D
	}
}

namespace Hazel {

	using NodeBuilder = ax::NodeEditor::Utilities::BlueprintNodeBuilder;

	struct SelectAssetPinContext
	{
		UUID PinID = 0;
		AssetHandle SelectedAssetHandle = 0;
		AssetType AssetType = AssetType::None;
	};

	class NodeEditorDraw
	{
	public:
		static bool PropertyBool(choc::value::Value& value);
		static bool PropertyFloat(choc::value::Value& value);
		static bool PropertyInt(choc::value::Value& value);
		static bool PropertyString(choc::value::Value& value);
		static bool PropertyEnum(int enumValue, Pin* pin, bool& openEnumPopup,
			std::function<choc::value::Value(int)> constructValue = [](int selected) {return choc::value::createInt32(selected);});

		template<typename T, AssetType assetType>
		static bool PropertyAsset(choc::value::Value& value, Pin* inputPin, bool& openAssetPopup)
		{
			HZ_CORE_VERIFY(inputPin);

			bool modified = false;

			AssetHandle selected = 0;
			Ref<T> asset;

			// TODO: position popup below the property button
			//auto canvaslPos = ImGui::GetCursorScreenPos();
			//auto screenPos = ed::ScreenToCanvas(canvaslPos);

			bool assetDropped = false;
			ImGui::SetNextItemWidth(90.0f);

			if (Utils::IsAssetHandle<assetType>(value))
			{
				selected = Utils::GetAssetHandleFromValue(value);
			}

			if (AssetManager::IsAssetHandleValid(selected))
			{
				asset = AssetManager::GetAsset<T>(selected);
			}
			// Initialize choc::Value class object with correct asset type using helper function
			if (UI::AssetReferenceDropTargetButton("Asset", asset, assetType, assetDropped))
			{
				//HZ_CORE_WARN("Asset clicked");
				openAssetPopup = true;

				s_SelectAssetContext = { inputPin->ID, selected, assetType };
			}

			if (assetDropped)
			{
				value = Utils::CreateAssetHandleValue<assetType>(asset->Handle);
				modified = true;
			}

			return modified;
		}

	public:
		inline static SelectAssetPinContext s_SelectAssetContext;

	};


	class NodeGraphEditorBase : public AssetEditor
	{
	public:
		NodeGraphEditorBase(const char* id);
		virtual ~NodeGraphEditorBase();

		// If you override this, make sure to call base class method from subclass.
		void SetAsset(const Ref<Asset>& asset) override;

		std::vector<UUID> GetSelectedNodeIDs() const;

	protected:
		virtual bool InitializeEditor();

		void Render() override final;

		// Called after drawing main canvas.
		// Use this to draw extra windows.
		virtual void OnRender() {};

		// Called before ending draw of main canvas.
		// Use this to draw more stuff on the canvas.
		virtual void OnRenderOnCanvas(ImVec2 min, ImVec2 max) {};

		bool DeactivateInputIfDragging(const bool wasActive);

		virtual NodeEditorModel* GetModel() = 0;
		virtual const NodeEditorModel* GetModel() const = 0;
		virtual const char* GetIconForSimpleNode(const std::string& simpleNodeIdentifier) const { return nullptr; }

		void OnOpen() override {};
		void OnClose() override; // If you override this, make sure to call base class method from subclass.

		void LoadSettings();

		virtual void InitializeEditorStyle(ax::NodeEditor::Style& style);
		virtual int GetPinIconSize() { return 24; }

		struct PinPropertyContext
		{
			Pin* pin = nullptr;
			NodeEditorModel* model = nullptr;
			bool OpenAssetPopup = false;
			bool OpenEnumPopup = false;
		};
		virtual bool DrawPinPropertyEdit(PinPropertyContext& context);

		bool IsPinAcceptingLink(const Pin* pin) { return m_AcceptingLinkPins.first == pin || m_AcceptingLinkPins.second == pin; }
		ImColor GetIconColor(int pinTypeID) const;

		ImGuiWindowClass* GetWindowClass() { return &m_WindowClass; }
		void EnsureWindowIsDocked(ImGuiWindow* childWindow);

		std::pair<ImVec2, ImVec2> DrawGraph(const char* id);
		virtual void DrawNodes(PinPropertyContext& pinContext);
		virtual void DrawLinks();

		virtual void DrawNode(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext);
		virtual void DrawCommentNode(Node* node);

		virtual void DrawNodeHeader(Node* node, NodeBuilder& builder);
		virtual void DrawNodeMiddle(Node* node, NodeBuilder& builder);
		virtual void DrawNodeInputs(Node* node, NodeBuilder& builder, PinPropertyContext& pinContext);
		virtual void DrawNodeOutputs(Node* node, NodeBuilder& builder);
		virtual void DrawPinIcon(const Pin* pin, bool connected, int alpha);

		virtual void DrawNodeContextMenu(Node* node);
		virtual void DrawPinContextMenu(Pin* pin);
		virtual void DrawLinkContextMenu(Link* link);

		virtual void DrawDeferredComboBoxes(PinPropertyContext& pinContext);

	protected:
		Ref<Texture2D> m_NodeBuilderTexture;
		ax::NodeEditor::EditorContext* m_Editor = nullptr;
		bool m_Initialized = false;
		bool m_FirstFrame = true;

		// If subclass graph editor supports drag-drop onto canvas
		// then the drop targets can be checked and accepted as appropriate in this callback
		std::function<void ()> onDragDropOntoCanvas = nullptr;

		// If subclass graph editor has any custom nodes,
		// the popup UI items for them can be added on this callback
		std::function<Node* (bool searching, std::string_view searchedString)> onNodeListPopup = nullptr;

		// If subclass graph editor wants to do custom action when new node is created
		// from node popup, it can be done in this callback.
		std::function<void (Node* node)> onNewNodeFromPopup = nullptr;

		// If subclass graph editor wants to do custom action if new node popup is cancelled
		// it can be done in this callback.
		std::function<void()> onNewNodePopupCancelled = nullptr;

		// A pair of nodes that are accepting a link connection in this frame
		std::pair<Pin*, Pin*> m_AcceptingLinkPins{ nullptr, nullptr };

		// Render local states
		struct ContextState;
		ContextState* m_State = nullptr;

		// For debugging
		bool m_ShowNodeIDs = false;
		bool m_ShowSortIndices = false;

	private:
		std::string m_DockSpaceName = "NodeEditor";
		std::string m_CanvasName;
		ImGuiWindowClass m_WindowClass;
		ImGuiID m_MainDockID = 0;
		std::unordered_map<ImGuiID, ImGuiID> m_DockIDs;

		std::string m_GraphStatePath;
	};


	class IONodeGraphEditor : public NodeGraphEditorBase
	{
	public:
		IONodeGraphEditor(const char* id);
		virtual ~IONodeGraphEditor();

	protected:
		NodeEditorModel* GetModel() override { return m_Model.get(); }
		const NodeEditorModel* GetModel() const override { return m_Model.get(); }
		virtual void SetModel(Scope<IONodeEditorModel> model) { m_Model = std::move(model); ClearSelection(); }

		void OnRender() override;

		ImGuiWindowFlags GetWindowFlags() override;
		void OnWindowStylePush() override;
		void OnWindowStylePop() override;

		const char* GetIconForSimpleNode(const std::string& simpleNodeIdentifier) const override;

		void SelectNode(UUID id);
		bool IsAnyNodeSelected() const;
		void ClearSelection();

		void DrawToolbar();
		void DrawDetailsPanel();
		void DrawNodeDetails(UUID nodeID);
		void DrawPropertyDetails(EPropertyType propertyType, std::string_view propertyName);
		virtual void DrawGraphIO();
		virtual void DrawSelectedDetails();

	private:
		enum class ESelectionType
		{
			Invalid = 0,
			Input,
			Output,
			LocalVariable,
			Node
		};

		struct SelectedItem
		{
			ESelectionType SelectionType{ ESelectionType::Invalid };
			UUID ID = 0;

			std::string Name;

			operator bool() const;
			bool IsSelected(EPropertyType type, std::string_view name) const;

			/** Select property type */
			void Select(EPropertyType type, std::string_view name);

			void SelectNode(UUID id);
			void Clear();
			EPropertyType GetPropertyType() const;
			ESelectionType FromPropertyType(EPropertyType type) const;
		};

		Scope<IONodeEditorModel> m_Model = nullptr;
		Scope<SelectedItem> m_SelectedItem = nullptr;

		std::string m_ToolbarName;
		std::string m_GraphInputsName;
		std::string m_GraphDetailsName;
		std::string m_DetailsName;
	};

} // namespace Hazel

