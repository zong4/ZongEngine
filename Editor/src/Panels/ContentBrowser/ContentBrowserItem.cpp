
#include <cstdint>
#include "ContentBrowserItem.h"
#include "Panels/ContentBrowserPanel.h"

#include "Engine/Asset/AssetManager.h"
#include "Engine/Utilities/FileSystem.h"
#include "Engine/ImGui/ImGui.h"
#include "Engine/Editor/AssetEditorPanel.h"
#include "Engine/Editor/SelectionManager.h"

#include "Engine/Editor/EditorApplicationSettings.h"
#include "Engine/Editor/EditorResources.h"

// TEMP
#include "Engine/Asset/MeshRuntimeSerializer.h"

#include "imgui_internal.h"

namespace Engine {

	static char s_RenameBuffer[MAX_INPUT_BUFFER_LENGTH];

	ContentBrowserItem::ContentBrowserItem(ItemType type, AssetHandle id, const std::string& name, const Ref<Texture2D>& icon)
		: m_Type(type), m_ID(id), m_FileName(name), m_Icon(icon)
	{
		m_DisplayName = m_FileName;
		if (m_FileName.size() > 25)
			m_DisplayName = m_FileName.substr(0, 25) + "...";
	}

	void ContentBrowserItem::OnRenderBegin()
	{
		ImGui::PushID(&m_ID);
		ImGui::BeginGroup();
	}

	CBItemActionResult ContentBrowserItem::OnRender()
	{
		CBItemActionResult result;

		const auto& editorSettings = EditorApplicationSettings::Get();
		const float thumbnailSize = float(editorSettings.ContentBrowserThumbnailSize);
		const bool displayAssetType = editorSettings.ContentBrowserShowAssetTypes;

		SetDisplayNameFromFileName();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		const float edgeOffset = 4.0f;

		const float textLineHeight = ImGui::GetTextLineHeightWithSpacing() * 2.0f + edgeOffset * 2.0f;
		const float infoPanelHeight = std::max(displayAssetType ? thumbnailSize * 0.5f : textLineHeight, textLineHeight);

		const ImVec2 topLeft = ImGui::GetCursorScreenPos();
		const ImVec2 thumbBottomRight = { topLeft.x + thumbnailSize, topLeft.y + thumbnailSize };
		const ImVec2 infoTopLeft = { topLeft.x,				 topLeft.y + thumbnailSize };
		const ImVec2 bottomRight = { topLeft.x + thumbnailSize, topLeft.y + thumbnailSize + infoPanelHeight };

		auto drawShadow = [](const ImVec2& topLeft, const ImVec2& bottomRight, bool directory)
		{
			auto* drawList = ImGui::GetWindowDrawList();
			const ImRect itemRect = UI::RectOffset(ImRect(topLeft, bottomRight), 1.0f, 1.0f);
			drawList->AddRect(itemRect.Min, itemRect.Max, Colors::Theme::propertyField, 6.0f, directory ? 0 : ImDrawFlags_RoundCornersBottom, 2.0f);
		};

		const bool isFocused = ImGui::IsWindowFocused();

		const bool isSelected = SelectionManager::IsSelected(SelectionContext::ContentBrowser, m_ID);

		// Fill background
		//----------------

		if (m_Type != ItemType::Directory)
		{
			auto* drawList = ImGui::GetWindowDrawList();

			// Draw shadow
			drawShadow(topLeft, bottomRight, false);

			// Draw background
			drawList->AddRectFilled(topLeft, thumbBottomRight, Colors::Theme::backgroundDark);
			drawList->AddRectFilled(infoTopLeft, bottomRight, Colors::Theme::groupHeader, 6.0f, ImDrawFlags_RoundCornersBottom);
		}
		else if (ImGui::ItemHoverable(ImRect(topLeft, bottomRight), ImGui::GetID(&m_ID)) || isSelected)
		{
			// If hovered or selected directory

			// Draw shadow
			drawShadow(topLeft, bottomRight, true);

			auto* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(topLeft, bottomRight, Colors::Theme::groupHeader, 6.0f);
		}


		// Thumbnail
		//==========
		// TODO: replace with actual Asset Thumbnail interface

		ImGui::InvisibleButton("##thumbnailButton", ImVec2{ thumbnailSize, thumbnailSize });
		UI::DrawButtonImage(m_Icon, IM_COL32(255, 255, 255, 225),
			IM_COL32(255, 255, 255, 255),
			IM_COL32(255, 255, 255, 255),
			UI::RectExpanded(UI::GetItemRect(), -6.0f, -6.0f));

		// Info Panel
		//-----------

		auto renamingWidget = [&]
		{
			ImGui::SetKeyboardFocusHere();
			ImGui::InputText("##rename", s_RenameBuffer, MAX_INPUT_BUFFER_LENGTH);

			if (ImGui::IsItemDeactivatedAfterEdit() || Input::IsKeyDown(KeyCode::Enter))
			{
				Rename(s_RenameBuffer);
				m_IsRenaming = false;
				SetDisplayNameFromFileName();
				result.Set(ContentBrowserAction::Renamed, true);
			}
		};

		UI::ShiftCursor(edgeOffset, edgeOffset);
		if (m_Type == ItemType::Directory)
		{
			ImGui::BeginVertical((std::string("InfoPanel") + m_DisplayName).c_str(), ImVec2(thumbnailSize - edgeOffset * 2.0f, infoPanelHeight - edgeOffset));
			{
				// Centre align directory name
				ImGui::BeginHorizontal(m_FileName.c_str(), ImVec2(thumbnailSize - 2.0f, 0.0f));
				ImGui::Spring();
				{
					ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (thumbnailSize - edgeOffset * 3.0f));
					const float textWidth = std::min(ImGui::CalcTextSize(m_DisplayName.c_str()).x, thumbnailSize);
					if (m_IsRenaming)
					{
						ImGui::SetNextItemWidth(thumbnailSize - edgeOffset * 3.0f);
						renamingWidget();
					}
					else
					{
						ImGui::SetNextItemWidth(textWidth);
						ImGui::Text(m_DisplayName.c_str());
					}
					ImGui::PopTextWrapPos();
				}
				ImGui::Spring();
				ImGui::EndHorizontal();

				ImGui::Spring();
			}
			ImGui::EndVertical();
		}
		else
		{
			ImGui::BeginVertical((std::string("InfoPanel") + m_DisplayName).c_str(), ImVec2(thumbnailSize - edgeOffset * 3.0f, infoPanelHeight - edgeOffset));
			{
				ImGui::BeginHorizontal("label", ImVec2(0.0f, 0.0f));

				ImGui::SuspendLayout();
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + (thumbnailSize - edgeOffset * 2.0f));
				if (m_IsRenaming)
				{
					ImGui::SetNextItemWidth(thumbnailSize - edgeOffset * 3.0f);
					renamingWidget();
				}
				else
				{
					ImGui::Text(m_DisplayName.c_str());
				}
				ImGui::PopTextWrapPos();
				ImGui::ResumeLayout();

				ImGui::Spring();
				ImGui::EndHorizontal();
			}
			ImGui::Spring();
			if (displayAssetType)
			{
				UI::ShiftCursorX(edgeOffset);
				ImGui::BeginHorizontal("assetType", ImVec2(0.0f, 0.0f));
				ImGui::Spring();
				{
					const AssetMetadata& metadata = Project::GetEditorAssetManager()->GetMetadata(m_ID);
					std::string assetType = Utils::ToUpper(Utils::AssetTypeToString(metadata.Type));
					UI::ScopedColour textColour(ImGuiCol_Text, Colors::Theme::textDarker);
					if (thumbnailSize < 128)
					{
						UI::Fonts::PushFont("ExtraSmall");
						if (metadata.Type == AssetType::AnimationGraph) // because it isn't guaranteed to fit on all thumb sizes
							assetType = "ANIMGRAPH";
					}
					else
						UI::Fonts::PushFont("Small");
					ImGui::TextUnformatted(assetType.c_str());
					UI::Fonts::PopFont();
				}
				ImGui::EndHorizontal();

				ImGui::Spring(-1.0f, edgeOffset);
			}
			ImGui::EndVertical();
		}
		UI::ShiftCursor(-edgeOffset, -edgeOffset);

		if (!m_IsRenaming)
		{
			if (Input::IsKeyDown(KeyCode::F2) && isSelected && isFocused)
				StartRenaming();
		}
		
		ImGui::PopStyleVar(); // ItemSpacing

		// End of the Item Group
		//======================
		ImGui::EndGroup();

		// Draw outline
		//-------------

		if (isSelected || ImGui::IsItemHovered())
		{
			ImRect itemRect = UI::GetItemRect();
			auto* drawList = ImGui::GetWindowDrawList();

			if (isSelected)
			{
				const bool mouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsItemHovered();
				ImColor colTransition = UI::ColourWithMultipliedValue(Colors::Theme::selection, 0.8f);

				drawList->AddRect(itemRect.Min, itemRect.Max,
								  mouseDown ? ImColor(colTransition) : ImColor(Colors::Theme::selection), 6.0f,
								  m_Type == ItemType::Directory ? 0 : ImDrawFlags_RoundCornersBottom, 1.0f);
			}
			else // isHovered
			{
				if (m_Type != ItemType::Directory)
				{
					drawList->AddRect(itemRect.Min, itemRect.Max,
									  Colors::Theme::muted, 6.0f,
									  ImDrawFlags_RoundCornersBottom, 1.0f);
				}
			}
		}


		// Mouse Events handling
		//======================

		UpdateDrop(result);

		bool dragging = false;
		if (dragging = ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			m_IsDragging = true;

			const auto& selectionStack = SelectionManager::GetSelections(SelectionContext::ContentBrowser);
			if (!SelectionManager::IsSelected(SelectionContext::ContentBrowser, m_ID))
				result.Set(ContentBrowserAction::ClearSelections, true);

			auto& currentItems = ContentBrowserPanel::Get().GetCurrentItems();

			if (!selectionStack.empty())
			{
				for (const auto& selectedItemHandles : selectionStack)
				{
					size_t index = currentItems.FindItem(selectedItemHandles);
					if (index == ContentBrowserItemList::InvalidItem)
						continue;

					const auto& item = currentItems[index];
					UI::Image(item->GetIcon(), ImVec2(20, 20));
					ImGui::SameLine();
					const auto& name = item->GetName();
					ImGui::TextUnformatted(name.c_str());
				}

				ImGui::SetDragDropPayload("asset_payload", selectionStack.data(), sizeof(AssetHandle) * selectionStack.size());
			}

			result.Set(ContentBrowserAction::Selected, true);
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsItemHovered())
		{
			result.Set(ContentBrowserAction::Hovered, true);

			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !m_IsRenaming)
			{
				result.Set(ContentBrowserAction::Activated, true);
			}
			else
			{
				bool action = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
				const bool isSelected = SelectionManager::IsSelected(SelectionContext::ContentBrowser, m_ID);
				bool skipBecauseDragging = m_IsDragging && isSelected;
				if (action && !skipBecauseDragging)
				{
					if (m_JustSelected)
						m_JustSelected = false;

					if (isSelected && Input::IsKeyDown(KeyCode::LeftControl) && !m_JustSelected)
					{
						result.Set(ContentBrowserAction::Deselected, true);
					}

					if (!isSelected)
					{
						result.Set(ContentBrowserAction::Selected, true);
						m_JustSelected = true;
					}

					if (!Input::IsKeyDown(KeyCode::LeftControl) && !Input::IsKeyDown(KeyCode::LeftShift) && m_JustSelected)
						result.Set(ContentBrowserAction::ClearSelections, true);

					if (Input::IsKeyDown(KeyCode::LeftShift))
						result.Set(ContentBrowserAction::SelectToHere, true);
				}
			}
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
		if (ImGui::BeginPopupContextItem("CBItemContextMenu"))
		{
			result.Set(ContentBrowserAction::Selected, true);
			OnContextMenuOpen(result);
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();

		m_IsDragging = dragging;

		return result;
	}

	void ContentBrowserItem::OnRenderEnd()
	{
		ImGui::PopID();
		ImGui::NextColumn();
	}

	void ContentBrowserItem::StartRenaming()
	{
		if (m_IsRenaming)
			return;

		memset(s_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
		memcpy(s_RenameBuffer, m_FileName.c_str(), m_FileName.size());
		m_IsRenaming = true;
	}

	void ContentBrowserItem::StopRenaming()
	{
		m_IsRenaming = false;
		SetDisplayNameFromFileName();
		memset(s_RenameBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	void ContentBrowserItem::Rename(const std::string& newName)
	{
		OnRenamed(newName);
	}

	void ContentBrowserItem::SetDisplayNameFromFileName()
	{
		const auto& editorSettings = EditorApplicationSettings::Get();
		const float thumbnailSize = editorSettings.ContentBrowserThumbnailSize;

		int maxCharacters = 0.00152587f * (thumbnailSize * thumbnailSize); // 0.00152587f is a magic number that is gained from graphing this equation in desmos and setting the y=25 at x=128

		if (m_FileName.size() > maxCharacters)
			m_DisplayName = m_FileName.substr(0, maxCharacters) + " ...";
		else
			m_DisplayName = m_FileName;
	}

	void ContentBrowserItem::OnContextMenuOpen(CBItemActionResult& actionResult)
	{
		if (ImGui::MenuItem("Reload"))
			actionResult.Set(ContentBrowserAction::Reload, true);

		if (SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser) == 1 && ImGui::MenuItem("Rename"))
			actionResult.Set(ContentBrowserAction::StartRenaming, true);

		if (ImGui::MenuItem("Copy"))
			actionResult.Set(ContentBrowserAction::Copy, true);

		if (ImGui::MenuItem("Duplicate"))
			actionResult.Set(ContentBrowserAction::Duplicate, true);

		if (ImGui::MenuItem("Delete"))
			actionResult.Set(ContentBrowserAction::OpenDeleteDialogue, true);

		ImGui::Separator();

		if (ImGui::MenuItem("Show In Explorer"))
			actionResult.Set(ContentBrowserAction::ShowInExplorer, true);

		if (ImGui::MenuItem("Open Externally"))
			actionResult.Set(ContentBrowserAction::OpenExternal, true);

		RenderCustomContextItems();
	}

	ContentBrowserDirectory::ContentBrowserDirectory(const Ref<DirectoryInfo>& directoryInfo)
		: ContentBrowserItem(ContentBrowserItem::ItemType::Directory, directoryInfo->Handle, directoryInfo->FilePath.filename().string(), EditorResources::FolderIcon), m_DirectoryInfo(directoryInfo)
	{
	}

	ContentBrowserDirectory::~ContentBrowserDirectory()
	{
	}

	void ContentBrowserDirectory::OnRenamed(const std::string& newName)
	{
		auto target = Project::GetActive()->GetAssetDirectory() / m_DirectoryInfo->FilePath;
		auto destination = Project::GetActive()->GetAssetDirectory() / m_DirectoryInfo->FilePath.parent_path() / newName;

		if (Utils::ToLower(newName) == Utils::ToLower(target.filename().string()))
		{
			auto tmp = Project::GetActive()->GetAssetDirectory() / m_DirectoryInfo->FilePath.parent_path() / "TempDir";
			FileSystem::Rename(target, tmp);
			target = tmp;
		}

		if (!FileSystem::Rename(target, destination))
		{
			ZONG_CORE_ERROR("Couldn't rename {0} to {1}!", m_DirectoryInfo->FilePath.filename().string(), newName);
		}
	}

	void ContentBrowserDirectory::UpdateDrop(CBItemActionResult& actionResult)
	{
		if (SelectionManager::IsSelected(SelectionContext::ContentBrowser, m_ID))
			return;

		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_payload");

			if (payload)
			{
				auto& currentItems = ContentBrowserPanel::Get().GetCurrentItems();
				uint32_t count = payload->DataSize / sizeof(AssetHandle);

				for (uint32_t i = 0; i < count; i++)
				{
					AssetHandle assetHandle = *(((AssetHandle*)payload->Data) + i);
					size_t index = currentItems.FindItem(assetHandle);
					if (index != ContentBrowserItemList::InvalidItem)
					{
						if (currentItems[index]->Move(m_DirectoryInfo->FilePath))
						{
							actionResult.Set(ContentBrowserAction::Refresh, true);
							currentItems.erase(assetHandle);
						}
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	}

	void ContentBrowserDirectory::Delete()
	{
		bool deleted = FileSystem::DeleteFile(Project::GetActive()->GetAssetDirectory() / m_DirectoryInfo->FilePath);
		if (!deleted)
		{
			ZONG_CORE_ERROR("Failed to delete folder {0}", m_DirectoryInfo->FilePath);
			return;
		}

		for (auto asset : m_DirectoryInfo->Assets)
			Project::GetEditorAssetManager()->OnAssetDeleted(asset);
	}

	bool ContentBrowserDirectory::Move(const std::filesystem::path& destination)
	{
		bool wasMoved = FileSystem::MoveFile(Project::GetActive()->GetAssetDirectory() / m_DirectoryInfo->FilePath, Project::GetActive()->GetAssetDirectory() / destination);
		if (!wasMoved)
			return false;

		return true;
	}

	ContentBrowserAsset::ContentBrowserAsset(const AssetMetadata& assetInfo, const Ref<Texture2D>& icon)
		: ContentBrowserItem(ContentBrowserItem::ItemType::Asset, assetInfo.Handle, assetInfo.FilePath.stem().string(), icon), m_AssetInfo(assetInfo)
	{
	}

	ContentBrowserAsset::~ContentBrowserAsset()
	{

	}

	void ContentBrowserAsset::Delete()
	{
		auto filepath = Project::GetEditorAssetManager()->GetFileSystemPath(m_AssetInfo);
		bool deleted = FileSystem::DeleteFile(filepath);
		if (!deleted)
		{
			ZONG_CORE_ERROR("Couldn't delete {0}", m_AssetInfo.FilePath);
			return;
		}

		auto currentDirectory = ContentBrowserPanel::Get().GetDirectory(m_AssetInfo.FilePath.parent_path());
		currentDirectory->Assets.erase(std::remove(currentDirectory->Assets.begin(), currentDirectory->Assets.end(), m_AssetInfo.Handle), currentDirectory->Assets.end());

		Project::GetEditorAssetManager()->OnAssetDeleted(m_AssetInfo.Handle);
	}

	bool ContentBrowserAsset::Move(const std::filesystem::path& destination)
	{
		auto filepath = Project::GetEditorAssetManager()->GetFileSystemPath(m_AssetInfo);
		bool wasMoved = FileSystem::MoveFile(filepath, Project::GetActive()->GetAssetDirectory() / destination);
		if (!wasMoved)
		{
			ZONG_CORE_ERROR("Couldn't move {0} to {1}", m_AssetInfo.FilePath, destination);
			return false;
		}

		Project::GetEditorAssetManager()->OnAssetRenamed(m_AssetInfo.Handle, destination / filepath.filename());
		return true;
	}

	void ContentBrowserAsset::OnRenamed(const std::string& newName)
	{
		auto filepath = Project::GetEditorAssetManager()->GetFileSystemPath(m_AssetInfo);
		const std::string extension = filepath.extension().string();
		std::filesystem::path newFilepath = fmt::format("{0}\\{1}{2}", filepath.parent_path().string(), newName, extension);

		std::string targetName = fmt::format("{0}{1}", newName, extension);
		if (Utils::ToLower(targetName) == Utils::ToLower(filepath.filename().string()))
		{
			FileSystem::RenameFilename(filepath, "temp-rename");
			filepath = fmt::format("{0}\\temp-rename{1}", filepath.parent_path().string(), extension);
		}

		if (FileSystem::RenameFilename(filepath, newName))
		{
			// Update AssetManager with new name
			Project::GetEditorAssetManager()->OnAssetRenamed(m_AssetInfo.Handle, newFilepath);
		}
		else
		{
			ZONG_CORE_ERROR("Couldn't rename {0} to {1}!", filepath.filename().string(), newName);
		}
	}

}
