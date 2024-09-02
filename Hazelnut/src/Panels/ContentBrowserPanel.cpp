#include <cstdlib>
#include "ContentBrowserPanel.h"

#include "Hazel/Asset/AnimationAssetSerializer.h"
#include "Hazel/Asset/MeshColliderAsset.h"

#include "Hazel/Audio/Sound.h"

#include "Hazel/Core/Application.h"
#include "Hazel/Core/Input.h"

#include "Hazel/Editor/AssetEditorPanel.h"
#include "Hazel/Editor/EditorApplicationSettings.h"
#include "Hazel/Editor/EditorResources.h"
#include "Hazel/Editor/NodeGraphEditor/AnimationGraph/AnimationGraphAsset.h"
#include "Hazel/Editor/NodeGraphEditor/SoundGraph/SoundGraphAsset.h"
#include "Hazel/Editor/SelectionManager.h"

#include "Hazel/ImGui/CustomTreeNode.h"
#include "Hazel/ImGui/ImGuiWidgets.h"

#include "Hazel/Project/Project.h"

#include "Hazel/Script/ScriptAsset.h"

#include "Hazel/Renderer/MaterialAsset.h"

#include "Hazel/Scene/Entity.h"
#include "Hazel/Scene/Prefab.h"

#include "Hazel/Utilities/StringUtils.h"

#include <filesystem>
#include <imgui_internal.h>
#include <stack>
#include <set>

namespace Hazel {

	static const std::set<AssetType> s_SupportedThumbnailAssetTypes = {
		AssetType::Texture,
		AssetType::Material,
		AssetType::Mesh,
		AssetType::MeshSource,
		AssetType::StaticMesh,
		AssetType::EnvMap,
	};

	static bool IsThumbnailSupported(AssetType type)
	{
		return s_SupportedThumbnailAssetTypes.contains(type);
	}

	ContentBrowserPanel::ContentBrowserPanel()
		: m_Project(nullptr)
	{
		s_Instance = this;

		m_AssetIconMap[".fbx"] = EditorResources::FBXFileIcon;
		m_AssetIconMap[".obj"] = EditorResources::OBJFileIcon;
		m_AssetIconMap[".gltf"] = EditorResources::GLTFFileIcon;
		m_AssetIconMap[".glb"] = EditorResources::GLBFileIcon;
		m_AssetIconMap[".wav"] = EditorResources::WAVFileIcon;
		m_AssetIconMap[".mp3"] = EditorResources::MP3FileIcon;
		m_AssetIconMap[".ogg"] = EditorResources::OGGFileIcon;
		m_AssetIconMap[".cs"] = EditorResources::CSFileIcon;
		m_AssetIconMap[".png"] = EditorResources::PNGFileIcon;
		m_AssetIconMap[".jpg"] = EditorResources::JPGFileIcon;
		m_AssetIconMap[".hmaterial"] = EditorResources::MaterialFileIcon;
		m_AssetIconMap[".hscene"] = EditorResources::SceneFileIcon;
		m_AssetIconMap[".hprefab"] = EditorResources::PrefabFileIcon;
		m_AssetIconMap[".hanim"] = EditorResources::AnimationFileIcon;
		m_AssetIconMap[".hanimgraph"] = EditorResources::AnimationGraphFileIcon;
		m_AssetIconMap[".hmesh"] = EditorResources::MeshFileIcon;
		m_AssetIconMap[".hsmesh"] = EditorResources::StaticMeshFileIcon;
		m_AssetIconMap[".hmc"] = EditorResources::MeshColliderFileIcon;
		m_AssetIconMap[".hpm"] = EditorResources::PhysicsMaterialFileIcon;
		m_AssetIconMap[".hskel"] = EditorResources::SkeletonFileIcon;
		m_AssetIconMap[".hsoundc"] = EditorResources::SoundConfigFileIcon;
		m_AssetIconMap[".sound_graph"] = EditorResources::SoundGraphFileIcon;

		m_AssetIconMap[".ttf"] = EditorResources::FontFileIcon;
		m_AssetIconMap[".ttc"] = m_AssetIconMap.at(".ttf");
		m_AssetIconMap[".otf"] = m_AssetIconMap.at(".ttf");

		memset(m_SearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	AssetHandle ContentBrowserPanel::ProcessDirectory(const std::filesystem::path& directoryPath, const Ref<DirectoryInfo>& parent)
	{
		const auto& directory = GetDirectory(directoryPath);
		if (directory)
			return directory->Handle;

		Ref<DirectoryInfo> directoryInfo = Ref<DirectoryInfo>::Create();
		directoryInfo->Handle = AssetHandle();
		directoryInfo->Parent = parent;

		if (directoryPath == m_Project->GetAssetDirectory())
			directoryInfo->FilePath = "";
		else
			directoryInfo->FilePath = std::filesystem::relative(directoryPath, m_Project->GetAssetDirectory());

		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
			{
				AssetHandle subdirHandle = ProcessDirectory(entry.path(), directoryInfo);
				directoryInfo->SubDirectories[subdirHandle] = m_Directories[subdirHandle];
				continue;
			}

			auto metadata = Project::GetEditorAssetManager()->GetMetadata(std::filesystem::relative(entry.path(), m_Project->GetAssetDirectory()));
			if (!metadata.IsValid())
			{
				AssetType type = Project::GetEditorAssetManager()->GetAssetTypeFromPath(entry.path());
				if (type == AssetType::None)
					continue;

				metadata.Handle = Project::GetEditorAssetManager()->ImportAsset(entry.path());
			}

			// Failed to import
			if (!metadata.IsValid())
				continue;

			directoryInfo->Assets.push_back(metadata.Handle);
		}

		m_Directories[directoryInfo->Handle] = directoryInfo;
		return directoryInfo->Handle;
	}

	void ContentBrowserPanel::ChangeDirectory(Ref<DirectoryInfo>& directory)
	{
		if (!directory)
			return;

		m_UpdateNavigationPath = true;

		m_CurrentItems.Items.clear();

		if (strlen(m_SearchBuffer) == 0)
		{
			for (auto& [subdirHandle, subdir] : directory->SubDirectories)
				m_CurrentItems.Items.push_back(Ref<ContentBrowserDirectory>::Create(subdir));

			std::vector<AssetHandle> invalidAssets;
			for (auto assetHandle : directory->Assets)
			{
				AssetMetadata metadata = Project::GetEditorAssetManager()->GetMetadata(assetHandle);
				
				if (!metadata.IsValid())
					continue;

				auto itemIcon = EditorResources::FileIcon;

				auto extension = metadata.FilePath.extension().string();
				if (m_AssetIconMap.find(extension) != m_AssetIconMap.end())
					itemIcon = m_AssetIconMap[extension];

				m_CurrentItems.Items.push_back(Ref<ContentBrowserAsset>::Create(metadata, itemIcon));
			}
		}
		else
		{
			m_CurrentItems = Search(m_SearchBuffer, directory);
		}

		SortItemList();

		m_PreviousDirectory = directory;
		m_CurrentDirectory = directory;

		ClearSelections();
		GenerateThumbnails();
	}

	void ContentBrowserPanel::OnBrowseBack()
	{
		m_NextDirectory = m_CurrentDirectory;
		m_PreviousDirectory = m_CurrentDirectory->Parent;
		ChangeDirectory(m_PreviousDirectory);
	}

	void ContentBrowserPanel::OnBrowseForward()
	{
		ChangeDirectory(m_NextDirectory);
	}

	static float s_Padding = 2.0f;
	static bool s_OpenDeletePopup = false;
	static bool s_OpenNewScriptPopup = false;

	void ContentBrowserPanel::OnImGuiRender(bool& isOpen)
	{
		GenerateThumbnails();

		m_IsContentBrowserHovered = false;
		m_IsContentBrowserFocused = false;
		if (ImGui::Begin("Content Browser", &isOpen, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
		{
			m_IsContentBrowserHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
			m_IsContentBrowserFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

			UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
			UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

			UI::ScopedStyle cellPadding(ImGuiStyleVar_CellPadding, ImVec2(10.0f, 2.0f));

			ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable
				| ImGuiTableFlags_SizingFixedFit
				| ImGuiTableFlags_BordersInnerV;
			UI::PushID();
			if (ImGui::BeginTable(UI::GenerateID(), 2, tableFlags, ImVec2(0.0f, 0.0f)))
			{
				ImGui::TableSetupColumn("Outliner", 0, 300.0f);
				ImGui::TableSetupColumn("Directory Structure", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				// Content Outliner

				ImGui::BeginChild("##folders_common");
				{
					UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
					UI::ScopedColourStack itemBg(ImGuiCol_Header, IM_COL32_DISABLE,
													ImGuiCol_HeaderActive, IM_COL32_DISABLE);

					if (m_BaseDirectory)
					{
						// TODO(Yan): can we not sort this every frame?
						std::vector<Ref<DirectoryInfo>> directories;
						directories.reserve(m_BaseDirectory->SubDirectories.size());
						for (auto& [handle, directory] : m_BaseDirectory->SubDirectories)
							directories.emplace_back(directory);

						std::sort(directories.begin(), directories.end(), [](const auto& a, const auto& b)
						{
							return a->FilePath.stem().string() < b->FilePath.stem().string();
						});

						for (auto& directory : directories)
							RenderDirectoryHierarchy(directory);
					}

					// Draw side shadow
					ImRect windowRect = UI::RectExpanded(ImGui::GetCurrentWindow()->Rect(), 0.0f, 10.0f);
					ImGui::PushClipRect(windowRect.Min, windowRect.Max, false);
					UI::DrawShadowInner(EditorResources::ShadowTexture, 20, windowRect, 1.0f, windowRect.GetHeight() / 4.0f, false, true, false, false);
					ImGui::PopClipRect();
				}
				ImGui::EndChild();

				ImGui::TableSetColumnIndex(1);

				// Directory Content

				const float topBarHeight = 26.0f;
				const float bottomBarHeight = 32.0f;
				ImGui::BeginChild("##directory_structure", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetWindowHeight() - topBarHeight - bottomBarHeight));
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
					RenderTopBar(topBarHeight);
					ImGui::PopStyleVar();

					ImGui::Separator();

					ImGui::BeginChild("Scrolling");
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.35f));

						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
						if (ImGui::BeginPopupContextWindow(0, 1, false))
						{
							if (ImGui::BeginMenu("New"))
							{
								if (ImGui::MenuItem("Folder"))
								{
									std::filesystem::path filepath = FileSystem::GetUniqueFileName(Project::GetActiveAssetDirectory() / m_CurrentDirectory->FilePath / "New Folder");

									// NOTE(Peter): For some reason creating new directories through code doesn't trigger a file system change?
									bool created = FileSystem::CreateDirectory(filepath);

									if (created)
									{
										Refresh();
										const auto& directoryInfo = GetDirectory(m_CurrentDirectory->FilePath / filepath.filename());
										size_t index = m_CurrentItems.FindItem(directoryInfo->Handle);
										if (index != ContentBrowserItemList::InvalidItem)
										{
											SelectionManager::DeselectAll(SelectionContext::ContentBrowser);
											SelectionManager::Select(SelectionContext::ContentBrowser, directoryInfo->Handle);
											m_CurrentItems[index]->StartRenaming();
										}
									}
								}

								if (ImGui::MenuItem("Scene"))
									CreateAsset<Scene>("New Scene.hscene");

								if (ImGui::BeginMenu("Physics"))
								{
									if (ImGui::MenuItem("Mesh Collider"))
										CreateAsset<MeshColliderAsset>("New Mesh Collider.hmc");

									ImGui::EndMenu();
								}

								if (ImGui::MenuItem("Animation Graph"))
								{
									auto extension = Project::GetEditorAssetManager()->GetDefaultExtensionForAssetType(AssetType::AnimationGraph);
									auto animationGraphAsset = CreateAsset<AnimationGraphAsset>("New Animation Graph" + extension);
									HZ_CORE_VERIFY(AnimationGraphAssetSerializer::TryLoadData("Resources/Animation/EmptyAnimationGraph" + extension, animationGraphAsset));
									AssetImporter::Serialize(animationGraphAsset);
								}

								if (ImGui::MenuItem("Material"))
									CreateAsset<MaterialAsset>("New Material.hmaterial");

								if (ImGui::BeginMenu("Audio"))
								{
									if (ImGui::MenuItem("Sound Config"))
										CreateAsset<SoundConfig>("New Sound Config.hsoundc");

									if (ImGui::MenuItem("SoundGraph Sound"))
										CreateAsset<SoundGraphAsset>("New SoundGraph Sound.sound_graph");

									ImGui::EndMenu();
								}

								if (ImGui::MenuItem("Script"))
									s_OpenNewScriptPopup = true;

								ImGui::EndMenu();
							}

							if (ImGui::MenuItem("Import"))
							{
								std::filesystem::path filepath = FileSystem::OpenFileDialog();
								if (!filepath.empty())
								{
									FileSystem::CopyFile(filepath, Project::GetActiveAssetDirectory() / m_CurrentDirectory->FilePath);
									Refresh();
								}
							}

							if (ImGui::MenuItem("Refresh"))
								Refresh();

							if (ImGui::MenuItem("Copy", "Ctrl+C", nullptr, SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser) > 0))
								m_CopiedAssets.CopyFrom(SelectionManager::GetSelections(SelectionContext::ContentBrowser));

							if (ImGui::MenuItem("Paste", "Ctrl+V", nullptr, m_CopiedAssets.SelectionCount() > 0))
								PasteCopiedAssets();

							if (ImGui::MenuItem("Duplicate", "Ctrl+D", nullptr, SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser) > 0))
							{
								m_CopiedAssets.CopyFrom(SelectionManager::GetSelections(SelectionContext::ContentBrowser));
								PasteCopiedAssets();
							}

							ImGui::Separator();

							if (ImGui::MenuItem("Show in Explorer"))
								FileSystem::OpenDirectoryInExplorer(Project::GetActiveAssetDirectory() / m_CurrentDirectory->FilePath);

							ImGui::EndPopup();
						}
						ImGui::PopStyleVar(); // ItemSpacing

						const float paddingForOutline = 2.0f;
						const float scrollBarrOffset = 20.0f + ImGui::GetStyle().ScrollbarSize;
						float panelWidth = ImGui::GetContentRegionAvail().x - scrollBarrOffset;
						float cellSize = EditorApplicationSettings::Get().ContentBrowserThumbnailSize + s_Padding + paddingForOutline;
						int columnCount = (int)(panelWidth / cellSize);
						if (columnCount < 1) columnCount = 1;

						{
							const float rowSpacing = 12.0f;
							UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(paddingForOutline, rowSpacing));
							ImGui::Columns(columnCount, 0, false);

							UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);
							UI::ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
							RenderItems();
						}

						if (ImGui::IsWindowFocused() && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
							UpdateInput();

						ImGui::PopStyleColor(2);

						RenderDeleteDialogue();
						RenderNewScriptDialogue();
					}
					ImGui::EndChild();
				}
				ImGui::EndChild();

				if (ImGui::BeginDragDropTarget())
				{
					auto data = ImGui::AcceptDragDropPayload("scene_entity_hierarchy");
					if (data)
					{
						size_t count = data->DataSize / sizeof(UUID);

						for (size_t i = 0; i < count; i++)
						{
							UUID entityID = *(((UUID*)data->Data) + i);
							Entity entity = m_SceneContext->TryGetEntityWithUUID(entityID);

							if (!entity)
							{
								HZ_CORE_ERROR_TAG("ContentBrowser", "Failed to find Entity with ID {0} in current Scene context!", entityID);
								continue;
							}

							Ref<Prefab> prefab = CreateAsset<Prefab>(entity.Name() + ".hprefab");
							prefab->Create(entity);
						}
					}
					ImGui::EndDragDropTarget();
				}

				RenderBottomBar(bottomBarHeight);

				ImGui::EndTable();
			}

			UI::PopID();
		}

		ImGui::End();
	}

	void ContentBrowserPanel::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event) { return OnKeyPressedEvent(event); });
		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) { return OnMouseButtonPressed(event); });
	}

	void ContentBrowserPanel::OnProjectChanged(const Ref<Project>& project)
	{
		m_Directories.clear();
		m_CurrentItems.Clear();
		m_BaseDirectory = nullptr;
		m_CurrentDirectory = nullptr;
		m_NextDirectory = nullptr;
		m_PreviousDirectory = nullptr;
		SelectionManager::DeselectAll();
		m_BreadCrumbData.clear();

		m_Project = project;

		m_ThumbnailCache = Ref<ThumbnailCache>::Create();
		m_ThumbnailGenerator = Ref<ThumbnailGenerator>::Create();

		AssetHandle baseDirectoryHandle = ProcessDirectory(project->GetAssetDirectory().string(), nullptr);
		m_BaseDirectory = m_Directories[baseDirectoryHandle];
		ChangeDirectory(m_BaseDirectory);

		memset(m_SearchBuffer, 0, MAX_INPUT_BUFFER_LENGTH);
	}

	static bool s_ActivateSearchWidget = false;
	bool ContentBrowserPanel::OnKeyPressedEvent(KeyPressedEvent& e)
	{
		if (!m_IsContentBrowserFocused)
			return false;

		bool handled = false;

		if (Input::IsKeyDown(KeyCode::LeftControl))
		{
			switch (e.GetKeyCode())
			{
				case KeyCode::C:
				{
					m_CopiedAssets.CopyFrom(SelectionManager::GetSelections(SelectionContext::ContentBrowser));
					handled = true;
					break;
				}
				case KeyCode::V:
				{
					PasteCopiedAssets();
					handled = true;
					break;
				}
				case KeyCode::D:
				{
					m_CopiedAssets.CopyFrom(SelectionManager::GetSelections(SelectionContext::ContentBrowser));
					PasteCopiedAssets();
					handled = true;
					break;
				}
				case KeyCode::F:
				{
					s_ActivateSearchWidget = true;
					break;
				}
			}

			if (Input::IsKeyDown(KeyCode::LeftShift))
			{
				switch (e.GetKeyCode())
				{
					case KeyCode::N:
					{
						std::filesystem::path filepath = FileSystem::GetUniqueFileName(Project::GetActiveAssetDirectory() / m_CurrentDirectory->FilePath / "New Folder");

						// NOTE(Peter): For some reason creating new directories through code doesn't trigger a file system change?
						bool created = FileSystem::CreateDirectory(filepath);

						if (created)
						{
							Refresh();
							const auto& directoryInfo = GetDirectory(m_CurrentDirectory->FilePath / filepath.filename());
							size_t index = m_CurrentItems.FindItem(directoryInfo->Handle);
							if (index != ContentBrowserItemList::InvalidItem)
							{
								SelectionManager::DeselectAll(SelectionContext::ContentBrowser);
								SelectionManager::Select(SelectionContext::ContentBrowser, directoryInfo->Handle);
								m_CurrentItems[index]->StartRenaming();
							}
						}
						handled = true;
					}
					break;
				}
			}
		}

		if (Input::IsKeyDown(KeyCode::LeftAlt) || Input::IsKeyDown(KeyCode::RightAlt))
		{
			switch (e.GetKeyCode())
			{
				case KeyCode::Left:
				{
					OnBrowseBack();
					handled = true;
					break;
				}
				case KeyCode::Right:
				{
					OnBrowseForward();
					handled = true;
					break;
				}
			}
		}

		if (e.GetKeyCode() == KeyCode::Delete && SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser) > 0)
		{
			for (const auto& item : m_CurrentItems)
			{
				if (item->IsRenaming())
					return false;
			}

			s_OpenDeletePopup = true;
			handled = true;
		}

		return handled;
	}

	bool ContentBrowserPanel::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (!m_IsContentBrowserFocused)
			return false;

		bool handled = false;
		switch (e.GetMouseButton())
		{
			// Back button
			case MouseButton::Button3:
				OnBrowseBack();
				handled = true;
				break;
				// Back button
			case MouseButton::Button4:
				OnBrowseForward();
				handled = true;
				break;
		}
		return handled;
	}

	void ContentBrowserPanel::PasteCopiedAssets()
	{
		if (m_CopiedAssets.SelectionCount() == 0)
			return;

		auto GetUniquePath = [](const std::filesystem::path& fp)
		{
			int counter = 0;
			auto checkFileName = [&counter, &fp](auto checkFileName) -> std::filesystem::path
			{
				++counter;
				const std::string counterStr = [&counter] {
					if (counter < 10)
						return "0" + std::to_string(counter);
					else
						return std::to_string(counter);
				}();

				std::string basePath = Utils::RemoveExtension(fp.string()) + "_" + counterStr + fp.extension().string();
				if (std::filesystem::exists(basePath))
					return checkFileName(checkFileName);
				else
					return std::filesystem::path(basePath);
			};

			return checkFileName(checkFileName);
		};

		for (AssetHandle copiedAsset : m_CopiedAssets)
		{
			size_t assetIndex = m_CurrentItems.FindItem(copiedAsset);

			if (assetIndex == ContentBrowserItemList::InvalidItem)
				continue;

			const auto& item = m_CurrentItems[assetIndex];
			auto originalFilePath = Project::GetActiveAssetDirectory();

			if (item->GetType() == ContentBrowserItem::ItemType::Asset)
			{
				originalFilePath /= item.As<ContentBrowserAsset>()->GetAssetInfo().FilePath;
				auto filepath = GetUniquePath(originalFilePath);
				HZ_CORE_VERIFY(!std::filesystem::exists(filepath));
				std::filesystem::copy_file(originalFilePath, filepath);
			}
			else
			{
				originalFilePath /= item.As<ContentBrowserDirectory>()->GetDirectoryInfo()->FilePath;
				auto filepath = GetUniquePath(originalFilePath);
				HZ_CORE_VERIFY(!std::filesystem::exists(filepath));
				std::filesystem::copy(originalFilePath, filepath, std::filesystem::copy_options::recursive);
			}
		}

		Refresh();

		SelectionManager::DeselectAll();
		m_CopiedAssets.Clear();
	}

	Ref<DirectoryInfo> ContentBrowserPanel::GetDirectory(const std::filesystem::path& filepath) const
	{
		if (filepath.string() == "" || filepath.string() == ".")
			return m_BaseDirectory;

		for (const auto& [handle, directory] : m_Directories)
		{
			if (directory->FilePath == filepath)
				return directory;
		}

		return nullptr;
	}

	namespace UI {

		static bool TreeNode(const std::string& id, const std::string& label, ImGuiTreeNodeFlags flags = 0, const Ref<Texture2D>& icon = nullptr)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (window->SkipItems)
				return false;

			return ImGui::TreeNodeWithIcon(icon, window->GetID(id.c_str()), flags, label.c_str(), NULL);
		}
	}

	void ContentBrowserPanel::RenderDirectoryHierarchy(Ref<DirectoryInfo>& directory)
	{
		std::string name = directory->FilePath.filename().string();
		std::string id = name + "_TreeNode";
		bool previousState = ImGui::TreeNodeBehaviorIsOpen(ImGui::GetID(id.c_str()));

		// ImGui item height hack
		auto* window = ImGui::GetCurrentWindow();
		window->DC.CurrLineSize.y = 20.0f;
		window->DC.CurrLineTextBaseOffset = 3.0f;
		//---------------------------------------------

		const ImRect itemRect = { window->WorkRect.Min.x, window->DC.CursorPos.y,
								  window->WorkRect.Max.x, window->DC.CursorPos.y + window->DC.CurrLineSize.y };

		const bool isItemClicked = [&itemRect, &id]
		{
			if (ImGui::ItemHoverable(itemRect, ImGui::GetID(id.c_str())))
			{
				return ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
			}
			return false;
		}();

		const bool isWindowFocused = ImGui::IsWindowFocused();


		auto fillWithColour = [&](const ImColor& colour)
		{
			const ImU32 bgColour = ImGui::ColorConvertFloat4ToU32(colour);
			ImGui::GetWindowDrawList()->AddRectFilled(itemRect.Min, itemRect.Max, bgColour);
		};

		// Fill with light selection colour if any of the child entities selected
		auto checkIfAnyDescendantSelected = [&](Ref<DirectoryInfo>& directory, auto isAnyDescendantSelected) -> bool
		{
			if (directory->Handle == m_CurrentDirectory->Handle)
				return true;

			if (!directory->SubDirectories.empty())
			{
				for (auto& [childHandle, childDir] : directory->SubDirectories)
				{
					if (isAnyDescendantSelected(childDir, isAnyDescendantSelected))
						return true;
				}
			}

			return false;
		};

		const bool isAnyDescendantSelected = checkIfAnyDescendantSelected(directory, checkIfAnyDescendantSelected);
		const bool isActiveDirectory = directory->Handle == m_CurrentDirectory->Handle;

		ImGuiTreeNodeFlags flags = (isActiveDirectory ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanFullWidth;

		// Fill background
		//----------------
		if (isActiveDirectory || isItemClicked)
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
		else if (isAnyDescendantSelected)
		{
			fillWithColour(Colors::Theme::selectionMuted);
		}

		// Tree Node
		//----------

		bool open = UI::TreeNode(id, name, flags, EditorResources::FolderIcon);

		if (isActiveDirectory || isItemClicked)
			ImGui::PopStyleColor();

		// Fixing slight overlap
		UI::ShiftCursorY(3.0f);

		// Create Menu
		//------------
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::BeginMenu("New"))
			{
				if (ImGui::MenuItem("Folder"))
				{
					bool created = FileSystem::CreateDirectory(Project::GetActiveAssetDirectory() / directory->FilePath / "New Folder");
					if (created)
						Refresh();
				}

				if (ImGui::MenuItem("Scene"))
					CreateAssetInDirectory<Scene>("New Scene.hscene", directory);

				if (ImGui::MenuItem("Material"))
					CreateAssetInDirectory<MaterialAsset>("New Material.hmaterial", directory);

				if (ImGui::MenuItem("Sound Config"))
					CreateAssetInDirectory<SoundConfig>("New Sound Config.hsoundc", directory);

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Show in Explorer"))
				FileSystem::OpenDirectoryInExplorer(Project::GetActiveAssetDirectory() / directory->FilePath);

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar(); // ItemSpacing

		// Draw children
		//--------------

		if (open)
		{
			std::vector<Ref<DirectoryInfo>> directories;
			directories.reserve(m_BaseDirectory->SubDirectories.size());
			for (auto& [handle, directory] : directory->SubDirectories)
				directories.emplace_back(directory);

			std::sort(directories.begin(), directories.end(), [](const auto& a, const auto& b)
			{
				return a->FilePath.stem().string() < b->FilePath.stem().string();
			});

			for (auto& child : directories)
				RenderDirectoryHierarchy(child);
		}

		UpdateDropArea(directory);

		if (open != previousState && !isActiveDirectory)
		{
			if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.01f))
				ChangeDirectory(directory);
		}

		if (open)
			ImGui::TreePop();
	}

	void ContentBrowserPanel::RenderTopBar(float height)
	{
		ImGui::BeginChild("##top_bar", ImVec2(0, height));
		ImGui::BeginHorizontal("##top_bar", ImGui::GetWindowSize());
		{
			const float edgeOffset = 4.0f;

			// Navigation buttons
			{
				UI::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));

				auto contenBrowserButton = [height](const char* labelId, const Ref<Texture2D>& icon)
				{
					const ImU32 buttonCol = Colors::Theme::backgroundDark;
					const ImU32 buttonColP = UI::ColourWithMultipliedValue(Colors::Theme::backgroundDark, 0.8f);
					UI::ScopedColourStack buttonColours(ImGuiCol_Button, buttonCol,
														ImGuiCol_ButtonHovered, buttonCol,
														ImGuiCol_ButtonActive, buttonColP);

					const float iconSize = std::min(24.0f, height);
					const float iconPadding = 3.0f;
					const bool clicked = ImGui::Button(labelId, ImVec2(iconSize, iconSize));
					UI::DrawButtonImage(icon, Colors::Theme::textDarker,
						UI::ColourWithMultipliedValue(Colors::Theme::textDarker, 1.2f),
						UI::ColourWithMultipliedValue(Colors::Theme::textDarker, 0.8f),
						UI::RectExpanded(UI::GetItemRect(), -iconPadding, -iconPadding));

					return clicked;
				};

				if (contenBrowserButton("##back", EditorResources::BackIcon))
				{
					OnBrowseBack();
				}
				UI::SetTooltip("Previous directory");

				ImGui::Spring(-1.0f, edgeOffset);

				if (contenBrowserButton("##forward", EditorResources::ForwardIcon))
				{
					OnBrowseForward();
				}
				UI::SetTooltip("Next directory");

				ImGui::Spring(-1.0f, edgeOffset * 2.0f);

				if (contenBrowserButton("##refresh", EditorResources::RefreshIcon))
				{
					Refresh();
				}
				UI::SetTooltip("Refresh");
				if (contenBrowserButton("##clearThumbnailCache", EditorResources::ClearIcon))
				{
					m_ThumbnailCache->Clear();
				}
				UI::SetTooltip("Clear thumbnail cache");

				ImGui::Spring(-1.0f, edgeOffset * 2.0f);
			}

			// Search
			{
				UI::ShiftCursorY(2.0f);
				ImGui::SetNextItemWidth(200);

				if (s_ActivateSearchWidget)
				{
					ImGui::SetKeyboardFocusHere();
					s_ActivateSearchWidget = false;
				}

				if (UI::Widgets::SearchWidget<MAX_INPUT_BUFFER_LENGTH>(m_SearchBuffer))
				{
					if (strlen(m_SearchBuffer) == 0)
					{
						ChangeDirectory(m_CurrentDirectory);
					}
					else
					{
						m_CurrentItems = Search(m_SearchBuffer, m_CurrentDirectory);
						SortItemList();
					}
				}
				UI::ShiftCursorY(-2.0f);
			}

			if (m_UpdateNavigationPath)
			{
				m_BreadCrumbData.clear();

				Ref<DirectoryInfo> current = m_CurrentDirectory;
				while (current && current->Parent != nullptr)
				{
					m_BreadCrumbData.push_back(current);
					current = current->Parent;
				}

				std::reverse(m_BreadCrumbData.begin(), m_BreadCrumbData.end());
				m_UpdateNavigationPath = false;
			}

			// Breadcrumbs
			{
				UI::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[0]);
				UI::ScopedColour textColour(ImGuiCol_Text, Colors::Theme::textDarker);

				const std::string& assetsDirectoryName = m_Project->GetConfig().AssetDirectory;
				ImVec2 textSize = ImGui::CalcTextSize(assetsDirectoryName.c_str());
				const float textPadding = ImGui::GetStyle().FramePadding.y;
				if (ImGui::Selectable(assetsDirectoryName.c_str(), false, 0, ImVec2(textSize.x, textSize.y + textPadding)))
				{
					SelectionManager::DeselectAll(SelectionContext::ContentBrowser);
					ChangeDirectory(m_BaseDirectory);
				}
				UpdateDropArea(m_BaseDirectory);

				for (auto& directory : m_BreadCrumbData)
				{
					ImGui::Text("/");

					std::string directoryName = directory->FilePath.filename().string();
					ImVec2 textSize = ImGui::CalcTextSize(directoryName.c_str());
					if (ImGui::Selectable(directoryName.c_str(), false, 0, ImVec2(textSize.x, textSize.y + textPadding)))
					{
						SelectionManager::DeselectAll(SelectionContext::ContentBrowser);
						ChangeDirectory(directory);
					}

					UpdateDropArea(directory);
				}
			}

			// Settings button
			ImGui::Spring();
			if (UI::Widgets::OptionsButton())
			{
				ImGui::OpenPopup("ContentBrowserSettings");
			}
			UI::SetTooltip("Content Browser settings");


			if (UI::BeginPopup("ContentBrowserSettings"))
			{
				auto& editorSettings = EditorApplicationSettings::Get();

				bool saveSettings = ImGui::MenuItem("Show Asset Types", nullptr, &editorSettings.ContentBrowserShowAssetTypes);
				saveSettings |= ImGui::SliderInt("##thumbnail_size", &editorSettings.ContentBrowserThumbnailSize, 96, 512);
				UI::SetTooltip("Thumnail Size");

				if (saveSettings)
					EditorApplicationSettingsSerializer::SaveSettings();

				UI::EndPopup();
			}

		}
		ImGui::EndHorizontal();
		ImGui::EndChild();
	}

	void ContentBrowserPanel::RenderItems()
	{
		m_IsAnyItemHovered = false;

		// TODO(Peter): This method of handling actions isn't great... It's starting to become spaghetti...
		for (auto& item : m_CurrentItems)
		{
			item->OnRenderBegin();

			CBItemActionResult result = item->OnRender(this);

			if (result.IsSet(ContentBrowserAction::ClearSelections))
				ClearSelections();

			if (result.IsSet(ContentBrowserAction::Deselected))
				SelectionManager::Deselect(SelectionContext::ContentBrowser, item->GetID());

			if (result.IsSet(ContentBrowserAction::Selected))
				SelectionManager::Select(SelectionContext::ContentBrowser, item->GetID());

			if (result.IsSet(ContentBrowserAction::SelectToHere) && SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser) == 2)
			{
				size_t firstIndex = m_CurrentItems.FindItem(SelectionManager::GetSelection(SelectionContext::ContentBrowser, 0));
				size_t lastIndex = m_CurrentItems.FindItem(item->GetID());

				if (firstIndex > lastIndex)
				{
					size_t temp = firstIndex;
					firstIndex = lastIndex;
					lastIndex = temp;
				}

				for (size_t i = firstIndex; i <= lastIndex; i++)
					SelectionManager::Select(SelectionContext::ContentBrowser, m_CurrentItems[i]->GetID());
			}

			if (result.IsSet(ContentBrowserAction::StartRenaming))
				item->StartRenaming();

			if (result.IsSet(ContentBrowserAction::Copy))
				m_CopiedAssets.Select(item->GetID());

			if (result.IsSet(ContentBrowserAction::Reload))
				AssetManager::ReloadData(item->GetID());

			if (result.IsSet(ContentBrowserAction::OpenDeleteDialogue) && !item->IsRenaming())
			{
				s_OpenDeletePopup = true;
			}

			if (result.IsSet(ContentBrowserAction::ShowInExplorer))
			{
				if (item->GetType() == ContentBrowserItem::ItemType::Directory)
					FileSystem::ShowFileInExplorer(m_Project->GetAssetDirectory() / m_CurrentDirectory->FilePath / item->GetName());
				else
					FileSystem::ShowFileInExplorer(Project::GetEditorAssetManager()->GetFileSystemPath(Project::GetEditorAssetManager()->GetMetadata(item->GetID())));
			}

			if (result.IsSet(ContentBrowserAction::OpenExternal))
			{
				if (item->GetType() == ContentBrowserItem::ItemType::Directory)
					FileSystem::OpenExternally(m_Project->GetAssetDirectory() / m_CurrentDirectory->FilePath / item->GetName());
				else
					FileSystem::OpenExternally(Project::GetEditorAssetManager()->GetFileSystemPath(Project::GetEditorAssetManager()->GetMetadata(item->GetID())));
			}

			if (result.IsSet(ContentBrowserAction::Hovered))
				m_IsAnyItemHovered = true;

			item->OnRenderEnd();

			if (result.IsSet(ContentBrowserAction::Duplicate))
			{
				m_CopiedAssets.Select(item->GetID());
				PasteCopiedAssets();
				break;
			}

			if (result.IsSet(ContentBrowserAction::Renamed))
			{
				SelectionManager::DeselectAll(SelectionContext::ContentBrowser);
				Refresh();
				SortItemList();

				// NOTE (Tim): Calling the next line of code will cause the folder before the renaming to be selected as well, 
				// and so you will have a selection count of 2 whereas only one will be valid, so we will simply not have any selected after a rename.
				//SelectionManager::Select(SelectionContext::ContentBrowser, item->GetID()); 
				break;
			}

			if (result.IsSet(ContentBrowserAction::Activated))
			{
				if (item->GetType() == ContentBrowserItem::ItemType::Directory)
				{
					SelectionManager::DeselectAll(SelectionContext::ContentBrowser);
					ChangeDirectory(item.As<ContentBrowserDirectory>()->GetDirectoryInfo());
					break;
				}
				else
				{
					auto assetItem = item.As<ContentBrowserAsset>();
					const auto& assetMetadata = assetItem->GetAssetInfo();

					if (m_ItemActivationCallbacks.find(assetMetadata.Type) != m_ItemActivationCallbacks.end())
					{
						m_ItemActivationCallbacks[assetMetadata.Type](assetMetadata);
					}
					else
					{
						AssetEditorPanel::OpenEditor(AssetManager::GetAsset<Asset>(assetMetadata.Handle));
					}
				}
			}

			if (result.IsSet(ContentBrowserAction::Refresh))
			{
				Refresh();
				break;
			}
		}

		// This is a workaround an issue with ImGui: https://github.com/ocornut/imgui/issues/331
		if (s_OpenDeletePopup)
		{
			ImGui::OpenPopup("Delete");
			s_OpenDeletePopup = false;
		}

		if (s_OpenNewScriptPopup)
		{
			ImGui::OpenPopup("New Script");
			s_OpenNewScriptPopup = false;
		}
	}

	void ContentBrowserPanel::RenderBottomBar(float height)
	{
		UI::ScopedStyle childBorderSize(ImGuiStyleVar_ChildBorderSize, 0.0f);
		UI::ScopedStyle frameBorderSize(ImGuiStyleVar_FrameBorderSize, 0.0f);
		UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		UI::ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

		ImGui::BeginChild("##bottom_bar", ImVec2(0.0f, height));
		ImGui::BeginHorizontal("##bottom_bar");
		{
			size_t selectionCount = SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser);
			if (selectionCount == 1)
			{
				AssetHandle firstSelection = SelectionManager::GetSelection(SelectionContext::ContentBrowser, 0);
				const auto& assetMetadata = Project::GetEditorAssetManager()->GetMetadata(firstSelection);

				std::string filepath = "";
				if (assetMetadata.IsValid())
				{
					filepath = "Assets/" + assetMetadata.FilePath.string();
				}
				else if (m_Directories.find(firstSelection) != m_Directories.end())
				{
					filepath = "Assets/" + m_Directories[firstSelection]->FilePath.string();
				}

				std::replace(filepath.begin(), filepath.end(), '\\', '/');
				ImGui::TextUnformatted(filepath.c_str());
			}
			else if (selectionCount > 1)
			{
				ImGui::Text("%d items selected", selectionCount);
			}
		}
		ImGui::EndHorizontal();
		ImGui::EndChild();
	}

	void ContentBrowserPanel::Refresh()
	{
		m_CurrentItems.Clear();
		m_Directories.clear();

		Ref<DirectoryInfo> currentDirectory = m_CurrentDirectory;
		AssetHandle baseDirectoryHandle = ProcessDirectory(m_Project->GetAssetDirectory().string(), nullptr);
		m_BaseDirectory = m_Directories[baseDirectoryHandle];
		m_CurrentDirectory = GetDirectory(currentDirectory->FilePath);

		if (!m_CurrentDirectory)
			m_CurrentDirectory = m_BaseDirectory; // Our current directory was removed

		ChangeDirectory(m_CurrentDirectory);
	}

	void ContentBrowserPanel::UpdateInput()
	{
		if (!m_IsContentBrowserHovered)
			return;

		if ((!m_IsAnyItemHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) || Input::IsKeyDown(KeyCode::Escape))
			ClearSelections();

		//if (Input::IsKeyDown(KeyCode::Delete) && SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser) > 0)
		//	ImGui::OpenPopup("Delete");

		if (Input::IsKeyDown(KeyCode::F5))
			Refresh();
	}

	void ContentBrowserPanel::ClearSelections()
	{
		std::vector<AssetHandle> selectedItems = SelectionManager::GetSelections(SelectionContext::ContentBrowser);
		for (AssetHandle itemHandle : selectedItems)
		{
			SelectionManager::Deselect(SelectionContext::ContentBrowser, itemHandle);
			if (size_t index = m_CurrentItems.FindItem(itemHandle); index != ContentBrowserItemList::InvalidItem)
			{
				if (m_CurrentItems[index]->IsRenaming())
					m_CurrentItems[index]->StopRenaming();
			}
		}
	}

	static bool s_IsDeletingItems = false;
	bool rightButtonHovered = false;
	bool leftButtonHovered = false;
	void ContentBrowserPanel::RenderDeleteDialogue()
	{
		if (ImGui::BeginPopupModal("Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser) == 0)
				ImGui::CloseCurrentPopup();

			ImGui::Text("Are you sure you want to delete %d items?", SelectionManager::GetSelectionCount(SelectionContext::ContentBrowser));

			const float contentRegionWidth = ImGui::GetContentRegionAvail().x;
			const float buttonWidth = 60.0f;

			if (!rightButtonHovered)
			{
				rightButtonHovered = Input::IsKeyPressed(KeyCode::Left);
				leftButtonHovered = !rightButtonHovered;
			}
			if (!leftButtonHovered)
			{
				leftButtonHovered = Input::IsKeyPressed(KeyCode::Right);
				rightButtonHovered = !leftButtonHovered;
			}

			UI::ShiftCursorX(((contentRegionWidth - (buttonWidth * 2.0f)) / 2.0f) - ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::Button("Yes", ImVec2(buttonWidth, 0.0f)) || (rightButtonHovered && Input::IsKeyDown(KeyCode::Enter)))
			{
				s_IsDeletingItems = true;

				std::vector<AssetMetadata> deletedAssetMetadata;

				auto selectedItems = SelectionManager::GetSelections(SelectionContext::ContentBrowser);
				for (AssetHandle handle : selectedItems)
				{
					size_t index = m_CurrentItems.FindItem(handle);
					if (index == ContentBrowserItemList::InvalidItem)
						continue;

					deletedAssetMetadata.push_back(Project::GetEditorAssetManager()->GetMetadata(handle));

					m_CurrentItems[index]->Delete();
					m_CurrentItems.erase(handle);
				}

				for (auto& callback : m_AssetDeletedCallbacks)
				{
					for (const auto& deletedMetadata : deletedAssetMetadata)
						callback(deletedMetadata);
				}

				for (AssetHandle handle : selectedItems)
				{
					if (m_Directories.find(handle) != m_Directories.end())
						RemoveDirectory(m_Directories[handle]);
				}

				SelectionManager::DeselectAll(SelectionContext::ContentBrowser);
				Refresh();

				s_IsDeletingItems = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("No", ImVec2(buttonWidth, 0.0f)) || (leftButtonHovered && Input::IsKeyDown(KeyCode::Enter)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	void ContentBrowserPanel::RenderNewScriptDialogue()
	{
		static constexpr size_t MaxClassNameLength = 64 + 1;
		static constexpr size_t MaxClassNamespaceLength = 64 + 1;
		static char s_ScriptNameBuffer[MaxClassNameLength]{ 0 };
		static char s_ScriptNamespaceBuffer[MaxClassNamespaceLength]{ 0 };

		UI::ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 6.0f));

		ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f));
		if (ImGui::BeginPopupModal("New Script", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##ScriptNamespace", Project::GetActive()->GetConfig().DefaultNamespace.c_str(), s_ScriptNamespaceBuffer, MaxClassNamespaceLength);
			
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##ScriptName", "Class Name", s_ScriptNameBuffer, MaxClassNameLength);

			ImGui::Separator();

			const bool fileAlreadyExists = FileSystem::Exists(Project::GetActiveAssetDirectory() / m_CurrentDirectory->FilePath / (std::string(s_ScriptNameBuffer) + ".cs"));
			ImGui::BeginDisabled(fileAlreadyExists);
			if (ImGui::Button("Create"))
			{
				if (strlen(s_ScriptNamespaceBuffer) == 0)
					strcpy(s_ScriptNamespaceBuffer, Project::GetActive()->GetConfig().DefaultNamespace.c_str());

				if (strlen(s_ScriptNameBuffer) > 0)
				{
					CreateAsset<ScriptFileAsset>(std::string(s_ScriptNameBuffer) + ".cs", s_ScriptNamespaceBuffer, s_ScriptNameBuffer);
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndDisabled();

			ImGui::SameLine();

			if (ImGui::Button("Close"))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		if (!ImGui::IsPopupOpen("New Script") && strlen(s_ScriptNameBuffer) > 0)
		{
			memset(s_ScriptNameBuffer, 0, MaxClassNameLength);
			memset(s_ScriptNamespaceBuffer, 0, MaxClassNamespaceLength);
		}
	}

	void ContentBrowserPanel::RemoveDirectory(Ref<DirectoryInfo>& directory, bool removeFromParent)
	{
		if (directory->Parent && removeFromParent)
		{
			auto& childList = directory->Parent->SubDirectories;
			childList.erase(childList.find(directory->Handle));
		}

		for (auto& [handle, subdir] : directory->SubDirectories)
			RemoveDirectory(subdir, false);

		directory->SubDirectories.clear();
		directory->Assets.clear();

		m_Directories.erase(m_Directories.find(directory->Handle));
	}

	void ContentBrowserPanel::UpdateDropArea(const Ref<DirectoryInfo>& target)
	{
		if (target->Handle != m_CurrentDirectory->Handle && ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("asset_payload");

			if (payload)
			{
				uint32_t count = payload->DataSize / sizeof(AssetHandle);

				for (uint32_t i = 0; i < count; i++)
				{
					AssetHandle assetHandle = *(((AssetHandle*)payload->Data) + i);
					size_t index = m_CurrentItems.FindItem(assetHandle);
					if (index != ContentBrowserItemList::InvalidItem)
					{
						m_CurrentItems[index]->Move(target->FilePath);
						m_CurrentItems.erase(assetHandle);
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	}

	void ContentBrowserPanel::SortItemList()
	{
		std::sort(m_CurrentItems.begin(), m_CurrentItems.end(), [](const Ref<ContentBrowserItem>& item1, const Ref<ContentBrowserItem>& item2)
		{
			if (item1->GetType() == item2->GetType())
				return Utils::ToLower(item1->GetName()) < Utils::ToLower(item2->GetName());

			return (uint16_t)item1->GetType() < (uint16_t)item2->GetType();
		});
	}

	void ContentBrowserPanel::GenerateThumbnails()
	{
		struct Thumbnail
		{
			Ref<ContentBrowserItem> Item;
			AssetHandle Handle;
		};

		std::vector<Thumbnail> thumbnails;

		for (const auto& item : m_CurrentItems)
		{
			if (item->GetType() == ContentBrowserItem::ItemType::Asset)
			{
				AssetHandle handle = item->GetID();
				if (!AssetManager::IsAssetHandleValid(handle))
					continue;

				bool isThumbnailSupported = IsThumbnailSupported(AssetManager::GetAssetType(handle));
				if (!isThumbnailSupported)
					continue;

				// NOTE(Yan): checking file timestamp every frame? Good idea?
				if (!m_ThumbnailCache->IsThumbnailCurrent(handle))
				{
					// Queue thumbnail to be generated
					thumbnails.emplace_back(Thumbnail{ item, handle });
					break; // One thumbnail per frame
				}
			}
		}

		for (auto& thumbnail : thumbnails)
		{
			Ref<Image2D> thumbnailImage = m_ThumbnailGenerator->GenerateThumbnail(thumbnail.Handle);
			if (thumbnailImage)
				m_ThumbnailCache->SetThumbnailImage(thumbnail.Handle, thumbnailImage);
		}
	}

	ContentBrowserItemList ContentBrowserPanel::Search(const std::string& query, const Ref<DirectoryInfo>& directoryInfo)
	{
		ContentBrowserItemList results;
		std::string queryLowerCase = Utils::ToLower(query);

		for (auto& [handle, subdir] : directoryInfo->SubDirectories)
		{
			std::string subdirName = subdir->FilePath.filename().string();
			if (subdirName.find(queryLowerCase) != std::string::npos)
				results.Items.push_back(Ref<ContentBrowserDirectory>::Create(subdir));

			ContentBrowserItemList list = Search(query, subdir);
			results.Items.insert(results.Items.end(), list.Items.begin(), list.Items.end());
		}

		for (auto& assetHandle : directoryInfo->Assets)
		{
			auto asset = Project::GetEditorAssetManager()->GetMetadata(assetHandle);
			std::string filename = Utils::ToLower(asset.FilePath.filename().string());

			if (filename.find(queryLowerCase) != std::string::npos)
				results.Items.push_back(Ref<ContentBrowserAsset>::Create(asset, m_AssetIconMap.find(asset.FilePath.extension().string()) != m_AssetIconMap.end() ? m_AssetIconMap[asset.FilePath.extension().string()] : EditorResources::FileIcon));
		}

		return results;
	}

	ContentBrowserPanel* ContentBrowserPanel::s_Instance;

}
