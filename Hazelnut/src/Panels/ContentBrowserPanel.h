#pragma once

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Renderer/Texture.h"
#include "Hazel/ImGui/ImGui.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Core/Events/KeyEvent.h"

#include "ContentBrowser/ContentBrowserItem.h"

#include "Hazel/Editor/EditorPanel.h"

#include <map>
#include <mutex>

#define MAX_INPUT_BUFFER_LENGTH 128

namespace Hazel {

	class SelectionStack
	{
	public:
		void CopyFrom(const SelectionStack& other)
		{
			m_Selections.assign(other.begin(), other.end());
		}

		void CopyFrom(const std::vector<AssetHandle>& other)
		{
			m_Selections.assign(other.begin(), other.end());
		}

		void Select(AssetHandle handle)
		{
			if (IsSelected(handle))
				return;

			m_Selections.push_back(handle);
		}

		void Deselect(AssetHandle handle)
		{
			if (!IsSelected(handle))
				return;

			for (auto it = m_Selections.begin(); it != m_Selections.end(); it++)
			{
				if (handle == *it)
				{
					m_Selections.erase(it);
					break;
				}
			}
		}

		bool IsSelected(AssetHandle handle) const
		{
			for (const auto& selectedHandle : m_Selections)
			{
				if (selectedHandle == handle)
					return true;
			}

			return false;
		}

		void Clear()
		{
			m_Selections.clear();
		}

		size_t SelectionCount() const { return m_Selections.size(); }
		const AssetHandle* SelectionData() const { return m_Selections.data(); }

		AssetHandle operator[](size_t index) const
		{
			HZ_CORE_ASSERT(index >= 0 && index < m_Selections.size());
			return m_Selections[index];
		}

		std::vector<AssetHandle>::iterator begin() { return m_Selections.begin(); }
		std::vector<AssetHandle>::const_iterator begin() const { return m_Selections.begin(); }
		std::vector<AssetHandle>::iterator end() { return m_Selections.end(); }
		std::vector<AssetHandle>::const_iterator end() const { return m_Selections.end(); }

	private:
		std::vector<AssetHandle> m_Selections;
	};

	struct ContentBrowserItemList
	{
		static constexpr size_t InvalidItem = std::numeric_limits<size_t>::max();

		std::vector<Ref<ContentBrowserItem>> Items;

		std::vector<Ref<ContentBrowserItem>>::iterator begin() { return Items.begin(); }
		std::vector<Ref<ContentBrowserItem>>::iterator end() { return Items.end(); }
		std::vector<Ref<ContentBrowserItem>>::const_iterator begin() const { return Items.begin(); }
		std::vector<Ref<ContentBrowserItem>>::const_iterator end() const { return Items.end(); }

		Ref<ContentBrowserItem>& operator[](size_t index) { return Items[index]; }
		const Ref<ContentBrowserItem>& operator[](size_t index) const { return Items[index]; }

		ContentBrowserItemList() = default;

		ContentBrowserItemList(const ContentBrowserItemList& other)
			: Items(other.Items)
		{
		}

		ContentBrowserItemList& operator=(const ContentBrowserItemList& other)
		{
			Items = other.Items;
			return *this;
		}

		void Clear()
		{
			std::scoped_lock<std::mutex> lock(m_Mutex);
			Items.clear();
		}

		void erase(AssetHandle handle)
		{
			size_t index = FindItem(handle);
			if (index == InvalidItem)
				return;

			std::scoped_lock<std::mutex> lock(m_Mutex);
			auto it = Items.begin() + index;
			Items.erase(it);
		}

		size_t FindItem(AssetHandle handle)
		{
			if (Items.empty())
				return InvalidItem;

			std::scoped_lock<std::mutex> lock(m_Mutex);
			for (size_t i = 0; i < Items.size(); i++)
			{
				if (Items[i]->GetID() == handle)
					return i;
			}

			return InvalidItem;
		}

	private:
		std::mutex m_Mutex;
	};

	class ContentBrowserPanel : public EditorPanel
	{
	public:
		ContentBrowserPanel();

		virtual void OnImGuiRender(bool& isOpen) override;

		virtual void OnEvent(Event& e) override;
		virtual void OnProjectChanged(const Ref<Project>& project) override;
		virtual void SetSceneContext(const Ref<Scene>& context) override { m_SceneContext = context; }

		ContentBrowserItemList& GetCurrentItems() { return m_CurrentItems; }

		Ref<DirectoryInfo> GetDirectory(const std::filesystem::path& filepath) const;

		void RegisterItemActivateCallbackForType(AssetType type, const std::function<void(const AssetMetadata&)>& callback)
		{
			m_ItemActivationCallbacks[type] = callback;
		}

		void RegisterAssetCreatedCallback(const std::function<void(const AssetMetadata&)>& callback)
		{
			m_NewAssetCreatedCallbacks.push_back(callback);
		}

		void RegisterAssetDeletedCallback(const std::function<void(const AssetMetadata&)>& callback)
		{
			m_AssetDeletedCallbacks.push_back(callback);
		}

	public:
		static ContentBrowserPanel& Get() { return *s_Instance; }

	private:
		AssetHandle ProcessDirectory(const std::filesystem::path& directoryPath, const Ref<DirectoryInfo>& parent);
		
		void ChangeDirectory(Ref<DirectoryInfo>& directory);
		void OnBrowseBack();
		void OnBrowseForward();

		void RenderDirectoryHierarchy(Ref<DirectoryInfo>& directory);
		void RenderTopBar(float height);
		void RenderItems();
		void RenderBottomBar(float height);

		void Refresh();

		void UpdateInput();

		bool OnKeyPressedEvent(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void PasteCopiedAssets();

		void ClearSelections();

		void RenderDeleteDialogue();
		void RenderNewScriptDialogue();
		void RemoveDirectory(Ref<DirectoryInfo>& directory, bool removeFromParent = true);

		void UpdateDropArea(const Ref<DirectoryInfo>& target);

		void SortItemList();

		ContentBrowserItemList Search(const std::string& query, const Ref<DirectoryInfo>& directoryInfo);

	private:
		// NOTE: This should only be used within the ContentBrowserPanel!
		//		 For creating a new asset outside the content browser, use AssetManager::CreateNewAsset!
		template<typename T, typename... Args>
		Ref<T> CreateAsset(const std::string& filename, Args&&... args)
		{
			return CreateAssetInDirectory<T>(filename, m_CurrentDirectory, std::forward<Args>(args)...);
		}

		// NOTE: This should only be used within the ContentBrowserPanel!
		//		 For creating a new asset outside the content browser, use AssetManager::CreateNewAsset!
		template<typename T, typename... Args>
		Ref<T> CreateAssetInDirectory(const std::string& filename, Ref<DirectoryInfo>& directory, Args&&... args)
		{
			auto filepath = FileSystem::GetUniqueFileName(Project::GetAssetDirectory() / directory->FilePath / filename);

			Ref<T> asset = Project::GetEditorAssetManager()->CreateNewAsset<T>(filepath.filename().string(), directory->FilePath.string(), std::forward<Args>(args)...);
			if (!asset)
				return nullptr;

			directory->Assets.push_back(asset->Handle);

			const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(asset);
			for (auto& callback : m_NewAssetCreatedCallbacks)
				callback(metadata);

			Refresh();

			return asset;
		}

	private:
		Ref<Project> m_Project;
		Ref<Scene> m_SceneContext;

		std::map<std::string, Ref<Texture2D>> m_AssetIconMap;

		ContentBrowserItemList m_CurrentItems;

		Ref<DirectoryInfo> m_CurrentDirectory;
		Ref<DirectoryInfo> m_BaseDirectory;
		Ref<DirectoryInfo> m_NextDirectory, m_PreviousDirectory;

		bool m_IsAnyItemHovered = false;

		SelectionStack m_CopiedAssets;

		std::unordered_map<AssetHandle, Ref<DirectoryInfo>> m_Directories;

		std::unordered_map<AssetType, std::function<void(const AssetMetadata&)>> m_ItemActivationCallbacks;
		std::vector<std::function<void(const AssetMetadata&)>> m_NewAssetCreatedCallbacks;
		std::vector<std::function<void(const AssetMetadata&)>> m_AssetDeletedCallbacks;

		char m_SearchBuffer[MAX_INPUT_BUFFER_LENGTH];

		std::vector<Ref<DirectoryInfo>> m_BreadCrumbData;
		bool m_UpdateNavigationPath = false;

		bool m_IsContentBrowserHovered = false;
		bool m_IsContentBrowserFocused = false;

		bool m_ShowAssetType = true;

	private:
		static ContentBrowserPanel* s_Instance;
	};

}
