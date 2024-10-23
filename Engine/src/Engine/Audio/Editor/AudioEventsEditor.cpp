#include <hzpch.h>
#include "AudioEventsEditor.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "Engine/ImGui/ImGui.h"
#include "Engine/ImGui/Colors.h"
#include "Engine/ImGui/CustomTreeNode.h"

#include "Engine/Audio/AudioEvents/AudioCommandRegistry.h"
#include "Engine/Audio/AudioEvents/CommandID.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Core/Input.h"
#include "Engine/Renderer/Texture.h"

#include "yaml-cpp/yaml.h"

#include "Engine/Utilities/SerializationMacros.h"

#include "Engine/ImGui/ImGui.h"

#include "choc/text/choc_UTF8.h"
#include "choc/text/choc_Files.h"

#include <iomanip>


namespace Hazel {

	static bool PropertyDropdownNoLabel(const char* label, const std::vector<std::string>& options, int32_t optionCount, int32_t* selected)
	{
		const char* current = options[*selected].c_str();

		bool changed = false;

		std::string id = "##" + std::string(label);
		if (ImGui::BeginCombo(id.c_str(), current))
		{
			for (int i = 0; i < optionCount; i++)
			{
				bool is_selected = (current == options[i]);
				if (ImGui::Selectable(options[i].c_str(), is_selected))
				{
					current = options[i].c_str();
					*selected = i;
					changed = true;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		return changed;
	}

	template<typename T>
	static bool PropertyAssetReferenceNoLabel(const char* label, Ref<T>& object, AssetType supportedType, bool dropTargetEnabled = true)
	{
		bool modified = false;

		if (object)
		{
			if (!object->IsFlagSet(AssetFlag::Missing))
			{
				auto assetFileName = Utils::RemoveExtension(Project::GetEditorAssetManager()->GetMetadata(object->Handle).FilePath.filename().string());
				ImGui::InputText(UI::GenerateID(), (char*)assetFileName.c_str(), 256, ImGuiInputTextFlags_ReadOnly);
			}
			else
			{
				ImGui::InputText(UI::GenerateID(), "Missing", 256, ImGuiInputTextFlags_ReadOnly);
			}
		}
		else
		{
			ImGui::InputText(UI::GenerateID(), (char*)"Null", 256, ImGuiInputTextFlags_ReadOnly);
		}

		if (dropTargetEnabled)
		{
			if (ImGui::BeginDragDropTarget())
			{
				auto data = ImGui::AcceptDragDropPayload("asset_payload");

				if (data)
				{
					AssetHandle assetHandle = *(AssetHandle*)data->Data;
					Ref<Asset> asset = AssetManager::GetAsset<Asset>(assetHandle);
					if (asset && asset->GetAssetType() == T::GetStaticType())
					{
						object = asset.As<T>();
						modified = true;
					}
				}

				ImGui::EndDragDropTarget();
			}
		}

		return modified;
	}

	static bool NameProperty(const char* label, std::string& value, bool error = false, ImGuiInputTextFlags flags = 0)
	{
		bool modified = false;

		ImGui::Text(label);
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);

		char buffer[256 + 1] {};
		strncpy(buffer, value.c_str(), 256);

		//s_IDBuffer[0] = '#';
		//s_IDBuffer[1] = '#';
		//memset(s_IDBuffer + 2, 0, 14);
		//itoa(s_Counter++, s_IDBuffer + 2, 16);
		std::string id = "##" + std::string(label);

		if (error)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
		if (ImGui::InputText(id.c_str(), buffer, 256, flags))
		{
			value = buffer;
			modified = true;
		}
		if (error)
			ImGui::PopStyleColor();
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		return modified;
	}


	using namespace Audio;

	std::vector <AudioEventsEditor::ReorderData> AudioEventsEditor::s_MarkedForReorder;

	TreeModel<CommandID> AudioEventsEditor::s_Tree;

	ImGuiWindow* AudioEventsEditor::s_WindowHandle = nullptr;


	std::filesystem::path GetTreeViewPath(const std::filesystem::path& commandsRegistryPath)
	{
		std::filesystem::path treeViewPath = commandsRegistryPath;
		treeViewPath.replace_filename(treeViewPath.stem().string() + "_TreeView" + commandsRegistryPath.extension().string());
		return treeViewPath;
	}

	AudioEventsEditor::AudioEventsEditor()
	{
		AudioCommandRegistry::onRegistryChange = [this]
		{
			OnRegistryChanged();
		};
	}

	AudioEventsEditor::~AudioEventsEditor()
	{
		AudioCommandRegistry::onRegistryChange = nullptr;
		Reset();
	}

	void AudioEventsEditor::OnProjectChanged(const Ref<Project>& project)
	{
		if (!project)
		{
			// Active Project set to NONE, is this even valid??
			Serialize();

			Reset();
		}
		else
		{

			// Serialize current project tree view
			Serialize();

			Reset();

			// Deserialize new project tree view

			auto filepath = project->GetAudioCommandsRegistryPath();
			s_CurrentRegistryPath = filepath;

			const auto treeViewPath = GetTreeViewPath(s_CurrentRegistryPath);

			if (FileSystem::Exists(filepath) && !FileSystem::IsDirectory(filepath))
			{
				// We separated editor tree view from the ACR file,
				// but if it doesn't exist yet, we might have old version of ACR
				// and we want to try to load the tree from it.
				// TODO: JP. remove this at some point in the future (after next 'master' branch release)
				if (FileSystem::Exists(treeViewPath) && !FileSystem::IsDirectory(treeViewPath))
					filepath = treeViewPath;

				const std::string data = choc::file::loadFileAsString(filepath.string());
				std::vector<YAML::Node> documents = YAML::LoadAll(data);
				AudioEventsEditor::DeserializeTree(documents);
			}

		}
	}

	void AudioEventsEditor::OnOpennesChange(bool opened)
	{
		if (!opened)
			AudioCommandRegistry::Serialize();
	}

	void AudioEventsEditor::OnRegistryChanged()
	{
	}

	void AudioEventsEditor::Reset()
	{
		s_CurrentRegistryPath.clear();

		m_NewlyCreatedNode = nullptr;
		m_RenamingEntry = nullptr;
		m_Selection.Clear();
		m_MarkedForDelete.clear();
		s_MarkedForReorder.clear();

		m_Renaming = false;
		m_RenameConflict.clear();
		m_RenamingActive = false;

		s_Tree.RootNode.Children.clear();
	}

	bool AudioEventsEditor::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		switch (e.GetKeyCode())
		{
			case KeyCode::Escape:
				if (m_Renaming)
				{
					// If text renaming is active, deactivate it
					// Just clearing the flag, focus handled by ImGui
					m_Renaming = false;
					m_RenamingEntry = nullptr;
				}
				else if (!m_Selection.IsEmpty())
				{
					m_Selection.Clear();
				}
				return true;
				break;
			case KeyCode::Delete:
				if (!m_Selection.IsEmpty())
				{
					if (m_RenamingActive)
						return true;

					for (auto* item : m_Selection.GetVector())
						OnDeleteNode(item);
					m_Selection.Clear();
					return true;
				}
				break;
			case KeyCode::F2:
				ActivateRename(m_Selection.GetLast());
				return true;
				break;
		}
		//if (Input::IsKeyPressed(HZ_KEY_LEFT_SHIFT))
		//{
		//	switch (e.GetKeyCode())
		//	{
		//	case KeyCode::Up:
		//	{
		//		//? This messes up with default selection via arrow keys when focesed item changes
		//		
		//		//auto id = CommandID(m_SelectionContext.c_str());
		//		//s_OrderVector.SetNewPosition(id, s_OrderVector.GetIndex(id) - 1);
		//		return true;
		//		break;
		//	}
		//	case KeyCode::Down:
		//		//auto id = CommandID(m_SelectionContext.c_str());
		//		//s_OrderVector.SetNewPosition(id, s_OrderVector.GetIndex(id) + 1);
		//		return true;
		//		break;
		//	}
		//}

		return false;
	}


	void AudioEventsEditor::OnImGuiRender(bool& isOpen)
	{
		if (!isOpen)
		{
			if (m_Open)
			{
				m_Open = false;
				OnOpennesChange(false);
			}
			return;
		}

		if (!m_Open)
			OnOpennesChange(true);
		m_Open = true;

		ImGui::SetNextWindowSizeConstraints({ 600.0f, 400.0f }, ImGui::GetWindowContentRegionMax());

		ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::text);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, { 0.5f, 0.0f });
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 8.0f });
		ImGui::Begin("Audio Events Editor", &isOpen, ImGuiWindowFlags_NoCollapse);
		s_WindowHandle = ImGui::GetCurrentWindow();
		ImGui::PopStyleVar(2);

		m_IsWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		const auto width = ImGui::GetContentRegionAvail().x;

		ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::Theme::backgroundDark);
		ImGui::PushStyleColor(ImGuiCol_Border, Colors::Theme::backgroundDark);

		ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable
			| ImGuiTableFlags_SizingFixedFit
			| ImGuiTableFlags_PadOuterX
			| ImGuiTableFlags_NoHostExtendY
			| ImGuiTableFlags_BordersInnerV;

		UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 8.0f));

		UI::ShiftCursorX(4.0f);

		UI::PushID();
		if (ImGui::BeginTable(UI::GenerateID(), 2, tableFlags, ImVec2(ImGui::GetContentRegionAvail().x,
			ImGui::GetContentRegionAvail().y - 8.0f)))
		{
			ImGui::TableSetupColumn("Events", 0, 300.0f);
			ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			DrawEventsList();

			ImGui::TableSetColumnIndex(1);

			DrawSelectionDetails();

			ImGui::EndTable();
		}
		UI::PopID();

		ImGui::PopStyleColor(3);
		ImGui::End(); // Audio Evnets Editor

		s_WindowHandle = nullptr;
	}

	void AudioEventsEditor::OnEvent(Event& e)
	{
		auto isAudioEventsEditorFocused = []
		{
			// Note: child name usually starts with "parent window name"/"child name"
			auto* audioEventsEditorWindow = ImGui::FindWindowByName("Audio Events Editor");
			if (GImGui->NavWindow)
				return GImGui->NavWindow == audioEventsEditorWindow
				|| Utils::StartsWith(GImGui->NavWindow->Name, "Audio Events Editor");
			else
				return false;
		};

		if (!isAudioEventsEditorFocused())
			return;

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) { return OnKeyPressedEvent(e); });
	}

	void AudioEventsEditor::DrawTreeNode(CommandsTree::Node* treeNode, bool isRootNode /*= false*/)
	{
		// ImGui item height hack
		auto* window = ImGui::GetCurrentWindow();
		window->DC.CurrLineSize.y = 20.0f;
		window->DC.CurrLineTextBaseOffset = 3.0f;
		//---------------------------------------------

		const bool isFolder = !treeNode->Value.has_value();
		const bool isParentRootNode = treeNode->Parent == &treeNode->Tree->RootNode;

		// TODO: need Selection Manager!
		auto isSelected = [&] {
			for (auto s : m_Selection.GetVector())
			{
				if (isFolder)
				{
					if (s == treeNode)
						return true;
				}
				else if (s->Value)
				{
					if (*s->Value == *treeNode->Value)
						return true;
				}
			}
			return false;
		};

		bool selected = !isRootNode && isSelected();
		bool opened = false;
		bool deleteNode = false;

		if (isRootNode)
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Leaf;
			ImGuiItemFlags itemFlags = ImGuiItemFlags_NoNav | ImGuiItemFlags_NoNavDefaultFocus;
			ImGui::PushItemFlag(itemFlags, true);
			UI::BeginDisabled();
			opened = ImGui::TreeNodeWithIcon(nullptr, "##RootNode", flags);
			UI::EndDisabled();
			ImGui::PopItemFlag();
			ImGui::SetItemAllowOverlap();
			ImGui::SameLine();
			ImGui::Unindent();
		}
		else
		{
			const char* name = isFolder ? treeNode->Name.c_str() : [&]
			{
				CommandID commandID = *treeNode->Value;
				if (AudioCommandRegistry::DoesCommandExist<TriggerCommand>(commandID))
				{
					auto& command = AudioCommandRegistry::GetCommand<TriggerCommand>(commandID);
					return command.DebugName.c_str();
				}
				return "<NULL>";
			}();

			const ImRect itemRect = { window->WorkRect.Min.x, window->DC.CursorPos.y,
									  window->WorkRect.Max.x, window->DC.CursorPos.y + window->DC.CurrLineSize.y };

			const bool isHovering = ImGui::ItemHoverable(itemRect, ImGui::GetID(name));
			const bool isItemClicked = [&itemRect, &name, isHovering]
			{
				if (isHovering)
					return ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);

				return false;
			}();

			const bool isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			// TODO: refactor s_Selection to take Selectable interface

			ImGuiTreeNodeFlags flags = (selected ? ImGuiTreeNodeFlags_Selected : 0)
				| ImGuiTreeNodeFlags_SpanFullWidth
				| (isFolder ? 0 : ImGuiTreeNodeFlags_Leaf);

			//if (s_RenamingEntry == commandID)
				//flags |= ImGuiTreeNodeFlags_AllowItemOverlap;

			//if(!Event.IsValid())
				//ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.3f, 1.0f));


			// Fill background
			//----------------

			auto fillWithColour = [&](const ImColor& colour)
			{
				// starting from WorkRect.Min.x starts from the very left, 
				// which can be used later when can fill selection and hover rect from the very left as well
				/*ImRect frame;
				frame.Min.x = window->DC.CursorPos.x; //window->WorkRect.Min.x;
				frame.Min.y = window->DC.CursorPos.y;
				frame.Max.x = window->WorkRect.Max.x;
				frame.Max.y = window->DC.CursorPos.y + window->DC.CurrLineSize.y;*/
				ImGui::GetWindowDrawList()->AddRectFilled(itemRect.Min, itemRect.Max, colour);
			};

			// If any descendant node selected, fill this folder with light colour
			if (selected || isItemClicked)
			{
				if (isWindowFocused)
					fillWithColour(Colors::Theme::selection);
				else
				{
					const ImColor col = UI::ColourWithMultipliedValue(Colors::Theme::selection, 0.8f);
					fillWithColour(UI::ColourWithMultipliedSaturation(col, 0.7f));
				}

				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Theme::backgroundDark);
			}
			else if (isFolder && !selected)
			{
				auto isAnyDescendantSelected = [&](CommandsTree::Node* treeNode, auto isAnyDescendantSelected) -> bool
				{
					if (!treeNode->Children.empty())
					{
						for (auto& [name, childNode] : treeNode->Children)
						{
							if (isAnyDescendantSelected(childNode.Raw(), isAnyDescendantSelected))
								return true;
						}
					}
					else
					{
						return m_Selection.Contains(treeNode);
					}

					return false;
				};

				if (isAnyDescendantSelected(treeNode, isAnyDescendantSelected))
				{
					fillWithColour(Colors::Theme::selectionMuted);
				}
			}


			// Tree Node
			//----------

			auto isDescendant = [](CommandsTree::Node* descendant, CommandsTree::Node* treeNode, auto isDescendant) -> bool
			{
				const bool isFolder = !treeNode->Value.has_value();
				if (!isFolder)
				{
					return false;
				}
				else
				{
					if (treeNode->Children.find(descendant->Name) != treeNode->Children.end())
						return true;

					for (auto& [name, childNode] : treeNode->Children)
					{
						if (isDescendant(descendant, childNode.Raw(), isDescendant))
							return true;
					}
				}

				return false;
			};

			if (isFolder)
			{
				// If we just created node inside of this folder, need to open the folder to activate renaming for the new node

				if (m_NewlyCreatedNode != nullptr && isDescendant(m_NewlyCreatedNode, treeNode, isDescendant))
				{
					ImGui::SetNextItemOpen(true);
				}
				opened = ImGui::TreeNodeWithIcon(EditorResources::FolderIcon, ImGui::GetID(name), flags, name, nullptr);
			}
			else
			{
				opened = ImGui::TreeNodeWithIcon(nullptr, ImGui::GetID(name), flags, name, nullptr);
			}

			// Draw an outline around the last selected item which details are now displayed
			if (selected && m_Selection.GetLast() == treeNode && isWindowFocused)
			{
				const ImU32 frameColour = Colors::Theme::text;
				ImGui::PushStyleColor(ImGuiCol_Border, frameColour);
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
				ImGui::RenderFrameBorder(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 0.0f);
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			}


			if (selected || isItemClicked)
				ImGui::PopStyleColor();


			// Handle selection
			//-----------------

			const bool itemClicked = ImGui::IsMouseReleased(ImGuiMouseButton_Left)
				&& ImGui::IsItemHovered()
				&& !ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f);

			// Activate newly created item and its name text input
			const bool entryWasJustCreated = m_NewlyCreatedNode == treeNode;
			if (entryWasJustCreated)
			{
				m_Selection.Clear();
				m_Selection.PushBack(treeNode);
			}
			else if (itemClicked || UI::NavigatedTo())
			{
				if (Input::IsKeyDown(KeyCode::LeftControl))
				{
					if (selected)
						m_Selection.Erase(treeNode);
					else
						m_Selection.PushBack(treeNode);
				}
				else if (Input::IsKeyDown(KeyCode::LeftShift))
				{
					// Select all items between the first one selected and this one (within this parent folder?)

					auto& siblings = treeNode->Parent->Children;
					auto selectionStart = siblings.find(m_Selection.GetFirst()->Name);

					// Only allowing Shift select within the same folder

					if (selectionStart != siblings.end())
					{
//						auto& thisNode = siblings.find(treeNode->Name);

						auto compare = siblings.key_comp();
						const bool before = compare(treeNode->Name, selectionStart->first);
						const bool after = compare(selectionStart->first, treeNode->Name);

						if (before || after)
							m_Selection.Clear();

						if (before)
						{
							auto end = siblings.find(treeNode->Name);
							auto rend = std::make_reverse_iterator(end);
							auto rstart = std::make_reverse_iterator(++selectionStart);

							for (auto& it = rstart; it != rend; it++)
							{
								m_Selection.PushBack(it->second.Raw());
							}
						}
						else if (after)
						{
							auto end = siblings.upper_bound(treeNode->Name);
							for (auto it = siblings.lower_bound(selectionStart->first); it != end; it++)
							{
								m_Selection.PushBack(it->second.Raw());
							}
						}
					}
				}
				else // Clicked with no modifiers
				{
					m_Selection.Clear();
					m_Selection.PushBack(treeNode);
				}

				//OnSelectionChanged(commandID); //? not used
			}


			// Context menu
			//-------------

			if (ImGui::BeginPopupContextItem())
			{
				// TODO: add "add event", "add folder" options

				if (ImGui::MenuItem("Rename", "F2"))
				{
					// This prevents loosing selection when Right-Clicking to rename not selected entry
					if (Input::IsKeyDown(KeyCode::LeftControl))
					{
						if (!m_Selection.Contains(treeNode))
							m_Selection.PushBack(treeNode);
					}

					ActivateRename(treeNode);
				}

				if (ImGui::MenuItem("Delete", "Delete"))
					deleteNode = true;

				ImGui::EndPopup();
			}


			// Drag & Drop
			//------------

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::Text(name); // TODO: draw children names, or number of as well?

				auto& map = treeNode->Parent->Children;
				auto it = map.find(treeNode->Name);

				HZ_CORE_ASSERT(it != map.end());

				Ref<CommandsTree::Node>& nodeRef = it->second;
				ImGui::SetDragDropPayload("audio_events_editor", &nodeRef, sizeof(Ref<CommandsTree::Node>));
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("audio_events_editor", isFolder ? 0 : ImGuiDragDropFlags_AcceptNoDrawDefaultRect);

				if (payload)
				{
					Ref<CommandsTree::Node> droppedHandle = *(Ref<CommandsTree::Node>*)payload->Data;

					ReorderData reorderData
					{
						droppedHandle,
						isFolder ? treeNode : treeNode->Parent,
					};

					s_MarkedForReorder.push_back(reorderData);
				}

				ImGui::EndDragDropTarget();
			}
		}


		//--- Draw Children ---
		//---------------------

		if (opened)
		{
			if (isFolder)
			{
				// First draw nested folders
				for (auto& [name, childNode] : treeNode->Children)
				{
					const bool childIsFolder = !childNode->Value.has_value();

					if (childIsFolder)
						DrawTreeNode(childNode.Raw());
				}

				// Then draw nested Events
				for (auto& [name, childNode] : treeNode->Children)
				{
					const bool childIsFolder = !childNode->Value.has_value();

					if (!childIsFolder)
						DrawTreeNode(childNode.Raw());
				}
			}

			ImGui::TreePop();
		}


		// Defer deletion until end of node UI
		if (deleteNode)
		{
			if (isSelected())
			{
				m_Selection.Erase(treeNode);
			}

			OnDeleteNode(treeNode);
		}
	}

	void AudioEventsEditor::OnSelectionChanged(CommandID commandID)
	{
	}

	void AudioEventsEditor::OnDeleteNode(CommandsTree::Node* treeNode)
	{
		// Mark for deletion and delete after entries all has been drawn
		m_MarkedForDelete.push_back(treeNode);
	}

	std::string AudioEventsEditor::GetUniqueName()
	{
		int counter = 0;
		auto checkID = [&counter](auto checkID) -> std::string
		{
			++counter;
			const std::string counterStr = [&counter] {
				if (counter < 10)
					return "0" + std::to_string(counter);
				else
					return std::to_string(counter);
			}();  // Pad with 0 if < 10;

			std::string idstr = "New_Event_" + counterStr;
			CommandID id = CommandID::FromString(idstr.c_str());
			if (AudioCommandRegistry::DoesCommandExist<TriggerCommand>(id))
				return checkID(checkID);
			else
			{
				return idstr;
			}
		};

		return checkID(checkID);
	}

	std::string AudioEventsEditor::GetUniqueFolderName(CommandsTree::Node* parentNode, const std::string& nameBase)
	{
		int counter = 0;
		auto checkName = [&counter, &parentNode, &nameBase](auto checkName) -> std::string
		{
			++counter;
			const std::string counterStr = [&counter] {
				if (counter < 10)
					return "0" + std::to_string(counter);
				else
					return std::to_string(counter);
			}();  // Pad with 0 if < 10;

			std::string idstr = nameBase + "_" + counterStr;

			auto nameTaken = [&parentNode](const std::string name) {
				const auto& siblings = parentNode->Children;
				return siblings.find(name) != siblings.end();
			};

			if (nameTaken(idstr))
				return checkName(checkName);
			else
			{
				return idstr;
			}
		};

		return checkName(checkName);
	}

	void AudioEventsEditor::ActivateRename(CommandsTree::Node* treeNode)
	{
		m_Renaming = true;
		// Used to make renaming text field active
		m_RenamingEntry = treeNode;
	}

	void AudioEventsEditor::OnEntryRenamed(CommandsTree::Node* treeNode, const char* newName)
	{
		CommandID oldCommandID = *treeNode->Value;

		if (AudioCommandRegistry::AddNewCommand<TriggerCommand>(newName))
		{
			CommandID newID = CommandID::FromString(newName);
			auto& newCommand = AudioCommandRegistry::GetCommand<TriggerCommand>(newID);

			// Copy old command content into the new command
			newCommand = AudioCommandRegistry::GetCommand<TriggerCommand>(oldCommandID);

			// We copied old name, so need to change it back to the new one
			newCommand.DebugName = newName;

			// Remove old command
			AudioCommandRegistry::RemoveCommand<TriggerCommand>(oldCommandID);

			auto* parent = treeNode->Parent;
			auto nodeHandle = parent->Children.extract(treeNode->Name);

			treeNode->Value = newID;
			treeNode->Name = newName;

			nodeHandle.key() = newName;
			bool inserted = parent->Children.insert(std::move(nodeHandle)).inserted;
			HZ_CORE_ASSERT(inserted);
		}
		else
		{
			HZ_CORE_ERROR("Trigger with the name {0} already exists! Audio command names must be unique.", newName);
		}

		m_Renaming = false;
		m_RenamingEntry = nullptr;
	}

	bool Hazel::AudioEventsEditor::IsRenaming()
	{
		return ImGui::GetActiveID() == ImGui::GetID("##eventname") || m_Renaming || m_RenamingEntry;
	}

	void AudioEventsEditor::DrawEventsList()
	{
		auto singleColumnSeparator = []
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 p = ImGui::GetCursorScreenPos();
			draw_list->AddLine(ImVec2(p.x - 9999, p.y), ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
		};

		if (ImGui::Button("New Event", { 80.0, 28.0f }))
		{
			const std::string name = GetUniqueName();
			AudioCommandRegistry::AddNewCommand<TriggerCommand>(name.c_str());

			auto id = CommandID(name.c_str());

			auto newNode = Ref<CommandsTree::Node>::Create(&s_Tree, id);
			newNode->Name = name;
			m_NewlyCreatedNode = newNode.Raw();

			if (!m_Selection.IsEmpty())
			{
				auto* selectedNode = m_Selection.GetLast();
				const bool isSelectedFolder = !selectedNode->Value.has_value();

				if (!isSelectedFolder)
				{
					// Add sibling node after this one
					auto* parent = selectedNode->Parent;
					parent->Children.emplace(name, newNode);
					newNode->Parent = parent;
				}
				else
				{
					// Add child node
					selectedNode->Children.emplace(name, newNode);
					newNode->Parent = selectedNode;
				}
			}
			else
			{
				s_Tree.RootNode.Children.emplace(name, newNode);
				newNode->Parent = &s_Tree.RootNode;
			}

			ActivateRename(newNode.Raw());
		}

		ImGui::SameLine();
		if (ImGui::Button("Add Folder", { 80.0, 28.0f }))
		{
			auto newFolderNode = Ref<CommandsTree::Node>::Create(&s_Tree);
			m_NewlyCreatedNode = newFolderNode.Raw();

			if (!m_Selection.IsEmpty())
			{
				auto* selectedNode = m_Selection.GetLast();
				if (selectedNode->Value)
				{
					// Add sibling node after this one
					auto* parent = selectedNode->Parent;
					newFolderNode->Parent = parent;
					newFolderNode->Name = GetUniqueFolderName(parent, "New Folder");
					parent->Children.emplace(newFolderNode->Name, newFolderNode);
				}
				else
				{
					// Add child node
					newFolderNode->Parent = selectedNode;
					newFolderNode->Name = GetUniqueFolderName(selectedNode, "New Folder");
					selectedNode->Children.emplace(newFolderNode->Name, newFolderNode);

				}
			}
			else
			{
				newFolderNode->Parent = &s_Tree.RootNode;
				newFolderNode->Name = GetUniqueFolderName(&s_Tree.RootNode, "New Folder");
				s_Tree.RootNode.Children.emplace(newFolderNode->Name, newFolderNode);
			}

			// TODO: activate renaming of the folder
		}

		//--- Handle items deletion ---
		//-----------------------------

		for (auto& node : m_MarkedForDelete)
		{
			auto deleteNode = [](CommandsTree::Node* treeNode, auto deleteNode) -> void
			{
				const bool isFolder = !treeNode->Value.has_value();
				if (isFolder)
				{
					for (auto& [name, childNode] : treeNode->Children)
						deleteNode(childNode.Raw(), deleteNode);

					treeNode->Children.clear();
				}
				else
				{
					CommandID id = *treeNode->Value;
					AudioCommandRegistry::RemoveCommand<TriggerCommand>(id);
				}
			};

			// Delete node and child nodes recursively
			deleteNode(node, deleteNode);

			node->Parent->Children.erase(node->Name);
		}
		m_MarkedForDelete.clear();

		ImGui::Spacing();
		ImGui::Spacing();

		ImGui::BeginChild("EventsList");
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 0.0f });


		//--- Draw the events Tree ---
		//----------------------------
		{
			UI::ScopedColourStack itemBg(ImGuiCol_Header, IM_COL32_DISABLE,
										 ImGuiCol_HeaderActive, IM_COL32_DISABLE);
			DrawTreeNode(&s_Tree.RootNode, true);
		}

		//--- Handle items reorder ---
		//----------------------------
		bool reorderResolved = true;

		for (auto& [node, newParent] : s_MarkedForReorder)
		{
			// TODO: handle folder name collisions!!!

			auto* oldParent = node->Parent;
			if (oldParent == newParent)
			{
				// If moved item was selected, move all other selected items to its parent folder
				if (m_Selection.Contains(node.Raw()))
				{
					for (auto& selectedNode : m_Selection.GetVector())
					{
						auto* sNodeParent = selectedNode->Parent;

						if (selectedNode != newParent && sNodeParent != newParent)
						{
							// If parent folder also selected, resolve move as if moving the parent folder
							if (m_Selection.Contains(selectedNode->Parent))
								continue;

							const bool isFolder = !selectedNode->Value.has_value();
							if (isFolder && newParent->Children.find(selectedNode->Name) != newParent->Children.end())
							{
								reorderResolved = CheckAndResolveNameConflict(newParent, selectedNode, sNodeParent);
							}
							else
							{
								newParent->Children.emplace(selectedNode->Name, selectedNode);
								sNodeParent->Children.erase(selectedNode->Name);
								selectedNode->Parent = newParent;
							}
						}
					}
				}
			}
			else
			{
				// If moved item was selected, move all selected items to the new parent folder

				if (m_Selection.Contains(node.Raw()))
				{
					// If moved item was selected, move all selected items


					for (auto& selectedNode : m_Selection.GetVector())
					{
						// If parent folder also selected, resolve move as if moving the parent folder
						if (m_Selection.Contains(selectedNode->Parent))
							continue;

						auto* sNodeParent = selectedNode->Parent;

						if (selectedNode != newParent && sNodeParent != newParent)
						{

							const bool isFolder = !selectedNode->Value.has_value();

							if (isFolder && newParent->Children.find(selectedNode->Name) != newParent->Children.end())
							{
								reorderResolved = CheckAndResolveNameConflict(newParent, selectedNode, sNodeParent);
							}
							else
							{
								newParent->Children.emplace(selectedNode->Name, selectedNode);
								sNodeParent->Children.erase(selectedNode->Name);
								selectedNode->Parent = newParent;
							}
						}
					}
				}
				else
				{
					const bool isFolder = !node->Value.has_value();

					if (isFolder && newParent->Children.find(node->Name) != newParent->Children.end())
					{
						reorderResolved = CheckAndResolveNameConflict(newParent, node.Raw(), oldParent);
					}
					else
					{
						newParent->Children.emplace(node->Name, node);
						oldParent->Children.erase(node->Name);
						node->Parent = newParent;
					}
				}
			}
		}

		if (reorderResolved)
			s_MarkedForReorder.clear();

		ImGui::PopStyleVar();
		ImGui::EndChild(); // EventsList

		// If dragged items dropped onto the background, move itmes up to the root node
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("audio_events_editor");

			if (payload)
			{
				Ref<CommandsTree::Node> droppedHandle = *(Ref<CommandsTree::Node>*)payload->Data;

				ReorderData reorderData
				{
					droppedHandle,
					&s_Tree.RootNode,
				};

				s_MarkedForReorder.push_back(reorderData);
			}

			ImGui::EndDragDropTarget();
		}
	}

	std::optional<std::string> AudioEventsEditor::CheckNameConflict(const std::string& name, CommandsTree::Node* parentNode, std::function<void()> onReplaceDestination, std::function<void()> onCancel)
	{
		// Setting default spacing values in case they were changed
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 4.0f });

		ImGui::OpenPopup("Name Conflict", ImGuiPopupFlags_NoOpenOverExistingPopup);

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2{ 400,0 });

		std::string conflictName = name;
		std::optional<std::string> resolvedName;

		if (ImGui::BeginPopupModal("Name Conflict", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("The destination contains folder with the same name.");
			ImGui::Separator();

			auto nameTaken = [parentNode](const std::string& name)
			{
				return parentNode->Children.find(name) != parentNode->Children.end();
			};

			ImGui::Spacing();

			UI::BeginPropertyGrid();
			if (NameProperty("Name", conflictName, true, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				if (!nameTaken(conflictName))
				{
					ImGui::CloseCurrentPopup();
					resolvedName = conflictName;
				}
			}

			UI::EndPropertyGrid();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			if (ImGui::Button("ASSIGN UNIQUE NAME"))
			{
				resolvedName = GetUniqueFolderName(parentNode, name);
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("REPLACE DESTINATION"))
			{
				onReplaceDestination();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			if (ImGui::Button("CANCEL"))
			{
				if (onCancel)
					onCancel();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

		return resolvedName;
	}

	bool AudioEventsEditor::CheckAndResolveNameConflict(CommandsTree::Node* newParent, CommandsTree::Node* node, CommandsTree::Node* oldParent)
	{
		bool resolved = false;

		auto onReplaceDestination = [&]
		{
			// If node name conflicts with its parent in the destination folder
			if (newParent->Children.at(node->Name) == oldParent)
			{
				// 1. Exctract node from the parent with the same name
				auto handle = oldParent->Children.extract(node->Name);

				// 2. Erase the parent from the destination folder
				newParent->Children.erase(node->Name);

				// 3. Insert moving node to the destination folder and assign new parent
				auto it = newParent->Children.insert(std::move(handle)).position;
				it->second->Parent = newParent;
			}
			else
			{
				newParent->Children.erase(node->Name);
				newParent->Children.emplace(node->Name, node);
				oldParent->Children.erase(node->Name);
				node->Parent = newParent;
			}


			resolved = true;
		};

		auto onCancel = [&resolved]
		{
			resolved = true;
		};

		if (auto newName = CheckNameConflict(node->Name, newParent, onReplaceDestination, onCancel))
		{
			auto handle = oldParent->Children.extract(node->Name);
			handle.key() = newName.value();
			auto it = newParent->Children.insert(std::move(handle)).position;
			it->second->Name = newName.value();
			it->second->Parent = newParent;

			resolved = true;
		}

		return resolved;
	}

	int name_edited_callback(ImGuiInputTextCallbackData* data)
	{
		if (data->EventChar == ' ')
			data->EventChar = '_';
		return 0;
	}

	void AudioEventsEditor::DrawEventDetails()
	{
		auto* window = ImGui::GetCurrentWindow();

		auto propertyGridSpacing = []
		{
			ImGui::Spacing();
			ImGui::NextColumn();
			ImGui::NextColumn();
		};
		auto singleColumnSeparator = []
		{
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 p = ImGui::GetCursorScreenPos();
			draw_list->AddLine(p/*ImVec2(p.x - 9999, p.y)*/, ImVec2(p.x + 9999, p.y), ImGui::GetColorU32(ImGuiCol_Border));
		};

		auto* selectedNode = m_Selection.GetLast();
		auto& trigger = AudioCommandRegistry::GetCommand<TriggerCommand>(*selectedNode->Value);

		//--- Event Name
		//--------------

		ImGui::Spacing();

		auto name = trigger.DebugName;
		char buffer[256 + 1] {};
		strncpy(buffer, name.c_str(), 256);
		ImGui::Text("Event Name: ");

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.08f,0.08f,0.08f,1.0f });

		// Fixing misalignment of name editing field and "Event Name: " Text
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);

		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_CallbackCharFilter;

		if (ImGui::InputText("##eventname", buffer, 256, flags, name_edited_callback))
		{
			OnEntryRenamed(m_Selection.GetLast(), buffer);
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);

		m_RenamingActive = IsRenaming();

		if (m_NewlyCreatedNode == selectedNode || m_RenamingEntry == selectedNode)
		{
			// Make text input active if this is a newly created event
			ImGui::SetKeyboardFocusHere(0);
			m_RenamingEntry = nullptr;
			m_NewlyCreatedNode = nullptr;
		}
		else
		{
			//f_Renaming = false;
		}

		ImGui::PopStyleColor();

		ImGui::Spacing();
		singleColumnSeparator();
		ImGui::Spacing();

		//--- Event Details
		//-----------------

		// TODO: need ImGui tables API!
		const float width = ImGui::GetColumnWidth();
		const float borderOffset = 7.0f;

		const float bottomAreaHeight = 50.0f;
		const float deleteButtonWidth = ImGui::GetFrameHeight();// 29.0f;
		const ImVec2 deleteButtonSize{ deleteButtonWidth, ImGui::GetFrameHeight() };

		const float moveButtonWidth = ImGui::GetFrameHeight();// 29.0f;
		const ImVec2 moveButtonSize{ moveButtonWidth, ImGui::GetFrameHeight() };

		const float tableWidth = width - borderOffset - moveButtonWidth - deleteButtonWidth - 5.5f; // 5.5 here ensures the jumpy "delete" button has some border
		const float numColumns = 3.0f;
		const float spacing = 2.0f;
		const float itemWidth = tableWidth / numColumns - spacing;
		const float headerWidth = tableWidth / numColumns;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + borderOffset + moveButtonWidth);
		ImGui::Text("Type");
		ImGui::SameLine(borderOffset + moveButtonWidth + spacing + headerWidth, spacing);
		ImGui::Text("Target");
		ImGui::SameLine(borderOffset + moveButtonWidth + spacing + headerWidth * 2.0f, spacing);
		ImGui::Text("Context");

		ImGui::Spacing();

		//--- Actions Scroll view
		//-----------------------

		auto actionsBounds = ImGui::GetContentRegionAvail();
		actionsBounds.x = width - borderOffset;
		actionsBounds.y -= bottomAreaHeight;
		ImGui::BeginChild("Actions", actionsBounds, false);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 1.0f });
		ImGui::Dummy({ width, 0.0f }); // 1px space offset

		const float scrollbarWidth = ImGui::GetCurrentWindow()->ScrollbarSizes.x;

		std::vector<std::pair<int, int>> actionsToReorder;

		for (int i = 0; i < (int)trigger.Actions.GetSize(); ++i)
		{
			std::string idstr = std::to_string(i);
			auto& action = trigger.Actions[i];
			ImGui::PushItemWidth(itemWidth);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.0f); // adding 1px "border"

			//--- Move Button

			auto stringID = std::string("MoveAction" + std::to_string(i));

			UI::ImageButton(stringID.c_str(), EditorResources::MoveIcon, ImVec2(ImGui::GetFrameHeight() - 12.0f, ImGui::GetFrameHeight() - 12.0f),
													ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 6, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), ImGui::ColorConvertU32ToFloat4(Colors::Theme::text));

			if (!ImGui::GetCurrentContext()->DragDropActive || ImGui::GetDragDropPayload()->SourceId == ImGui::GetItemID())
			{
				if (ImGui::BeginDragDropSource())
				{

					auto assetName = action.Target == nullptr ? "Null" : Project::GetEditorAssetManager()->GetMetadata(action.Target->Handle).FilePath.filename().string();

					const std::string dragTooltip = std::string(Utils::AudioActionTypeToString(action.Type))
						+ " | " + assetName
						+ " | " + std::string(Utils::AudioActionContextToString(action.Context));

					ImGui::SetDragDropPayload("audio_events_editor_action", &i, sizeof(int));

					ImGui::Text(dragTooltip.c_str()); // TODO: draw children names, or number of as well?
					// TODO: draw Action type icon

					ImGui::EndDragDropSource();
				}
			}

			//--- Type

			ImGui::SameLine(0.0f, spacing);
			int selectedType = (int)action.Type;
			if (PropertyDropdownNoLabel(("Type" + idstr).c_str(), { "Play", "Stop", "StopAll", "Pause", "PauseAll", "Resume", "ResumeAll" }, 7, &selectedType))
				action.Type = (EActionType)selectedType;

			//--- Target

			bool targetDisabled = false;
			if (action.Type == EActionType::PauseAll || action.Type == EActionType::ResumeAll
				|| action.Type == EActionType::StopAll || action.Type == EActionType::SeekAll)
			{
				UI::BeginDisabled();
				targetDisabled = true;
			}

			ImGui::SameLine(0.0f, spacing);

			auto isPayload = [](const char* payloadType) {
				if (auto* dragPayload = ImGui::GetDragDropPayload())
					return dragPayload->IsDataType(payloadType);
				return false;
			};

			// Sound Config field
			// ==================

			//PropertyAssetReferenceNoLabel(("Target" + idstr).c_str(), action.Target, AssetType::Audio, !targetDisabled && !isPayload("audio_events_editor_action"));
			bool dropped = false;
			if (UI::AssetReferenceDropTargetButton(("Target" + idstr).c_str(), action.Target, AssetType::SoundConfig, dropped, !targetDisabled && !isPayload("audio_events_editor_action")))
			{
				ImGui::OpenPopup(("AssetSearchPopup" + idstr).c_str());
			}

			ImGui::PopItemWidth();

			// Disable changed spacing for the popup
			ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

			AssetHandle assetHandle;
			if (action.Target)
				assetHandle = action.Target->Handle;

			bool clear = false;
			if (UI::Widgets::AssetSearchPopup(("AssetSearchPopup" + idstr).c_str(), AssetType::SoundConfig, assetHandle, &clear, "Search Sound Config"))
			{
				if (clear)
				{
					assetHandle = 0;
					action.Target = Ref<SoundConfig>();
				}
				else
				{
					action.Target = AssetManager::GetAsset<SoundConfig>(assetHandle);
				}
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 8.0f, 1.0f });

			// ==================

			if (targetDisabled)
				UI::EndDisabled();

			//--- Context

			// Disable context selection for Play and PostTrigger Action types,
			// they should only be used in GameObject context

			bool contextDisabled = false;
			if (action.Type == EActionType::Play || action.Type == EActionType::PostTrigger) // TODO: disable also for SetSwitch
			{
				UI::BeginDisabled();
				contextDisabled = true;
			}

			ImGui::SameLine(0.0f, spacing);
			ImGui::PushItemWidth(itemWidth - scrollbarWidth);
			int selectedContext = action.Type == EActionType::Play ? 0 : (int)action.Context;
			if (PropertyDropdownNoLabel(("Context" + idstr).c_str(), { "GameObject", "Global" }, 2, &selectedContext))
				action.Context = (EActionContext)selectedContext;
			ImGui::PopItemWidth();

			if (contextDisabled)
			{
				UI::EndDisabled();
			}

			//--- Delete Action Button

			ImGui::SameLine(0.0f, spacing);
			ImGui::PushID(std::string("DeleteAction" + std::to_string(i)).c_str());
			UI::ImageButton(EditorResources::ClearIcon, ImVec2(ImGui::GetFrameHeight() - 6.0f, ImGui::GetFrameHeight() - 6.0f),
									ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), 3, ImVec4(0.0f, 0.0f, 0.0f, 0.0f), ImGui::ColorConvertU32ToFloat4(Colors::Theme::textDarker));
			/*{
				// TODO: this should cause issues with itereation, somehow it doesn't
				trigger.Actions.ErasePosition(i);
			}*/
			//? For some reason clicks for ImageButton not registrering, so have to do this manual check
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				trigger.Actions.ErasePosition(i);
			}
			ImGui::PopID();

			//--- Drag & Drop Target

			ImVec2 dropTargetSize{ tableWidth + moveButtonWidth + deleteButtonWidth , ImGui::GetFrameHeight() };
			ImGui::SameLine(0.01f, 0.0f);
			ImGui::Dummy(dropTargetSize);
			if (isPayload("audio_events_editor_action"))
			{
				if (ImGui::BeginDragDropTarget())
				{
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("audio_events_editor_action");

					if (payload)
					{
						int droppedIndex = *(int*)payload->Data;
						actionsToReorder.push_back({ droppedIndex, i });
					}

					ImGui::EndDragDropTarget();
				}
			}
		}


		// Handle Action reorder
		for (auto& [source, destination] : actionsToReorder)
		{
			trigger.Actions.SetNewPosition(source, destination);
		}
		actionsToReorder.clear();

		ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing
		ImGui::EndChild();

		//-- Bottom Area
		//--------------

		//--- Add Action Button

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);

		const float butonHeight = 50.0f;
		auto posY = ImGui::GetWindowContentRegionMax().y - butonHeight;
		ImGui::SetCursorPosY(posY);

		ImGui::Spacing();
		ImGui::Spacing();
		if (ImGui::Button("Add Action"))
		{
			ImGui::OpenPopup("AddActionsPopup");
		}

		if (UI::BeginPopup("AddActionsPopup"))
		{
			if (ImGui::MenuItem("Play", nullptr, false))
			{
				TriggerAction triggerAction{ EActionType::Play, Ref<SoundConfig>(), EActionContext::GameObject };
				trigger.Actions.PushBack(triggerAction);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Stop", nullptr, false))
			{
				TriggerAction triggerAction{ EActionType::Stop, Ref<SoundConfig>(), EActionContext::GameObject };
				trigger.Actions.PushBack(triggerAction);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Pause", nullptr, false))
			{
				TriggerAction triggerAction{ EActionType::Pause, Ref<SoundConfig>(), EActionContext::GameObject };
				trigger.Actions.PushBack(triggerAction);
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Resume", nullptr, false))
			{
				TriggerAction triggerAction{ EActionType::Resume, Ref<SoundConfig>(), EActionContext::GameObject };
				trigger.Actions.PushBack(triggerAction);
				ImGui::CloseCurrentPopup();
			}
			UI::EndPopup();
		}
	}

	void AudioEventsEditor::DrawFolderDetails()
	{
		ImGui::Spacing();

		//--- Folder Name
		//---------------

		auto* selectedFolder = m_Selection.GetLast();
		auto* parent = selectedFolder->Parent;

		auto& name = selectedFolder->Name;
		char buffer[256 + 1] {};
		strncpy(buffer, name.c_str(), 256);
 		ImGui::Text("Folder Name: ");

		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0.08f,0.08f,0.08f,1.0f });

		// Fixing misalignment of name editing field and "Event Name: " Text
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);

		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll /*| ImGuiInputTextFlags_CallbackCharFilter*/;

		if (ImGui::InputText("##foldername", buffer, 256, flags, nullptr))
		{
			// Handle name conflict
			if (auto it = parent->Children.find(buffer) != parent->Children.end())
			{
				auto conflictNode = parent->Children.find(buffer)->second.Raw();
				if (conflictNode == selectedFolder)
					m_RenameConflict.clear();
				else
					m_RenameConflict = buffer;
			}
			else
			{
				auto handle = parent->Children.extract(name);
				handle.key() = buffer;
				auto pos = parent->Children.insert(std::move(handle)).position;
				pos->second->Name = buffer;
			}
		}
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);


		if (!m_RenameConflict.empty())
		{
			auto onReplaceDestination = [&]
			{
				parent->Children.erase(m_RenameConflict);

				auto handle = parent->Children.extract(name);
				handle.key() = m_RenameConflict;
				auto it = parent->Children.insert(std::move(handle)).position;
				it->second->Name = m_RenameConflict;

				m_RenameConflict.clear();
			};

			auto onCancel = [&]
			{
				m_RenameConflict.clear();
			};

			if (auto newName = CheckNameConflict(m_RenameConflict, parent, onReplaceDestination, onCancel))
			{
				auto handle = parent->Children.extract(name);
				handle.key() = newName.value();
				auto it = parent->Children.insert(std::move(handle)).position;
				it->second->Name = newName.value();

				m_RenameConflict.clear();
			}
		}

		if (m_NewlyCreatedNode == selectedFolder || m_RenamingEntry == selectedFolder)
		{
			// Make text input active if this is a newly created event
			ImGui::SetKeyboardFocusHere(0);
			m_RenamingEntry = nullptr;
			m_NewlyCreatedNode = nullptr;
		}
		else
		{
			//f_Renaming = false;
		}

		ImGui::PopStyleColor();
	}

	void AudioEventsEditor::DrawSelectionDetails()
	{
		if (m_Selection.IsEmpty())
			return;

		//? workaround for Right-click renaming entry that's not the last one added to the selection
		if (m_RenamingEntry != nullptr && m_RenamingEntry != m_Selection.GetLast())
		{
			if (!m_Selection.Contains(m_RenamingEntry))
			{
				// If Renaming Entry is not in selection, replace selection with Renaming Entry
				m_Selection.Clear();
				m_Selection.PushBack(m_RenamingEntry);
			}
			else
			{
				m_Selection.MoveToBack(m_RenamingEntry);
			}
		}

		if (auto* node = m_Selection.GetLast())
		{
			if (node->Value) DrawEventDetails();
			else			 DrawFolderDetails();
		}
	}

	void AudioEventsEditor::Serialize()
	{
		if (!s_CurrentRegistryPath.empty())
		{
			const auto treeViewPath = GetTreeViewPath(s_CurrentRegistryPath);

			YAML::Emitter out;
			SerializeTree(out);
			std::ofstream fout(treeViewPath);
			fout << out.c_str();
		}
	}

	void AudioEventsEditor::SerializeTree(YAML::Emitter& out)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "TriggersTreeView" << YAML::BeginSeq;

		auto serializeNode = [&out](CommandsTree::Node* treeNode, auto serializeNode) ->void
		{
			const bool isFolder = !treeNode->Value.has_value();
			if (isFolder)
			{
				// Serialize Folder

				out << YAML::BeginMap; // Folder entry
				out << YAML::Key << "Name" << YAML::Value << treeNode->Name;
				out << YAML::Key << "Children" << YAML::BeginSeq;

				for (auto& [name, childNode] : treeNode->Children)
					serializeNode(childNode.Raw(), serializeNode);

				out << YAML::EndSeq; // Children
				out << YAML::EndMap; // Folder entry
			}
			else
			{
				// Serialize Triger

				out << YAML::BeginMap; // Trigger entry
				HZ_SERIALIZE_PROPERTY(Name, treeNode->Name, out);
				HZ_SERIALIZE_PROPERTY(CommandID, (uint32_t)(*treeNode->Value), out);
				out << YAML::EndMap;

			}
		};

		serializeNode(&s_Tree.RootNode, serializeNode);

		out << YAML::EndSeq;// TriggersTreeView
		out << YAML::EndMap;
	}

	void AudioEventsEditor::DeserializeTree(std::vector<YAML::Node>& data)
	{
		s_Tree.RootNode.Children.clear();

		auto it = std::find_if(data.begin(), data.end(), [](const YAML::Node& doc) { return doc["TriggersTreeView"].IsDefined(); });

		if (it == data.end())
		{
			HZ_CORE_ERROR("Invalid, or missing TriggersTreeView cache.");
			return;
		}

		YAML::Node treeView = (*it)["TriggersTreeView"];

		auto deserializeNode = [](YAML::Node& node, CommandsTree::Node* parentFolder, auto deserializeNode) ->void
		{
			const bool isFolder = node["Children"].IsDefined();
			if (isFolder)
			{
				auto folder = Ref<CommandsTree::Node>::Create();

				folder->Tree = &s_Tree;
				HZ_DESERIALIZE_PROPERTY(Name, folder->Name, node, std::string(""));
				folder->Parent = parentFolder;

				for (auto childNode : node["Children"])
					deserializeNode(childNode, folder.Raw(), deserializeNode);

				parentFolder->Children.try_emplace(folder->Name, folder);
			}
			else
			{
				auto trigger = Ref<CommandsTree::Node>::Create();

				trigger->Tree = &s_Tree;
				HZ_DESERIALIZE_PROPERTY(Name, trigger->Name, node, std::string(""));
				trigger->Value = node["CommandID"] ? CommandID::FromUnsignedInt(node["CommandID"].as<uint32_t>()) : CommandID::InvalidID();
				trigger->Parent = parentFolder;

				parentFolder->Children.try_emplace(trigger->Name, trigger);
			}
		};

		auto rootNode = treeView[0];

		for (auto node : rootNode["Children"])
			deserializeNode(node, &s_Tree.RootNode, deserializeNode);
	}

} // namespace Hazel
