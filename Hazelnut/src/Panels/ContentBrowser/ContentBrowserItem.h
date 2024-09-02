#pragma once

#include "Hazel/Core/Input.h"
#include "Hazel/Renderer/Texture.h"
#include "Hazel/Asset/AssetMetadata.h"

#include <filesystem>
#include <map>

namespace Hazel {

#define MAX_INPUT_BUFFER_LENGTH 128


	enum class ContentBrowserAction
	{
		None                = 0,
		Refresh             = BIT(0),
		ClearSelections     = BIT(1),
		Selected			= BIT(2),
		Deselected			= BIT(3),
		Hovered             = BIT(4),
		Renamed             = BIT(5),
		OpenDeleteDialogue  = BIT(6),
		SelectToHere        = BIT(7),
		Moved               = BIT(8),
		ShowInExplorer      = BIT(9),
		OpenExternal        = BIT(10),
		Reload              = BIT(11),
		Copy				= BIT(12),
		Duplicate			= BIT(13),
		StartRenaming		= BIT(14),
		Activated			= BIT(15)
	};

	struct CBItemActionResult
	{
		uint16_t Field = 0;

		void Set(ContentBrowserAction flag, bool value)
		{
			if (value)
				Field |= (uint16_t)flag;
			else
				Field &= ~(uint16_t)flag;
		}

		bool IsSet(ContentBrowserAction flag) const { return (uint16_t)flag & Field; }
	};

	class ContentBrowserItem : public RefCounted
	{
	public:
		enum class ItemType : uint16_t
		{
			Directory, Asset
		};
	public:
		ContentBrowserItem(ItemType type, AssetHandle id, const std::string& name, const Ref<Texture2D>& icon);
		virtual ~ContentBrowserItem() {}

		void OnRenderBegin();
		CBItemActionResult OnRender();
		void OnRenderEnd();

		virtual void Delete() {}
		virtual bool Move(const std::filesystem::path& destination) { return false; }

		AssetHandle GetID() const { return m_ID; }
		ItemType GetType() const { return m_Type; }
		const std::string& GetName() const { return m_FileName; }

		const Ref<Texture2D>& GetIcon() const { return m_Icon; }

		void StartRenaming();
		void StopRenaming();
		bool IsRenaming() const { return m_IsRenaming; }

		void Rename(const std::string& newName);
		void SetDisplayNameFromFileName();

	private:
		virtual void OnRenamed(const std::string& newName) { m_FileName = newName; }
		virtual void RenderCustomContextItems() {}
		virtual void UpdateDrop(CBItemActionResult& actionResult) {}

		void OnContextMenuOpen(CBItemActionResult& actionResult);

	protected:
		ItemType m_Type;
		AssetHandle m_ID;
		std::string m_DisplayName;
		std::string m_FileName;
		Ref<Texture2D> m_Icon;

		bool m_IsRenaming = false;
		bool m_IsDragging = false;
		bool m_JustSelected = false;

	private:
		friend class ContentBrowserPanel;
	};

	struct DirectoryInfo : public RefCounted
	{
		AssetHandle Handle;
		Ref<DirectoryInfo> Parent;

		std::filesystem::path FilePath;

		std::vector<AssetHandle> Assets;

		std::map<AssetHandle, Ref<DirectoryInfo>> SubDirectories;
	};

	class ContentBrowserDirectory : public ContentBrowserItem
	{
	public:
		ContentBrowserDirectory(const Ref<DirectoryInfo>& directoryInfo);
		virtual ~ContentBrowserDirectory();

		Ref<DirectoryInfo>& GetDirectoryInfo() { return m_DirectoryInfo; }

		virtual void Delete() override;
		virtual bool Move(const std::filesystem::path& destination) override;

	private:
		virtual void OnRenamed(const std::string& newName) override;
		virtual void UpdateDrop(CBItemActionResult& actionResult) override;

		void UpdateDirectoryPath(Ref<DirectoryInfo> directoryInfo, const std::filesystem::path& newParentPath, const std::filesystem::path& newName);

	private:
		Ref<DirectoryInfo> m_DirectoryInfo;
	};

	class ContentBrowserAsset : public ContentBrowserItem
	{
	public:
		ContentBrowserAsset(const AssetMetadata& assetInfo, const Ref<Texture2D>& icon);
		virtual ~ContentBrowserAsset();

		const AssetMetadata& GetAssetInfo() const { return m_AssetInfo; }

		virtual void Delete() override;
		virtual bool Move(const std::filesystem::path& destination) override;

	private:
		virtual void OnRenamed(const std::string& newName) override;

	private:
		AssetMetadata m_AssetInfo;
	};

	namespace Utils
	{
		static std::string ContentBrowserItemTypeToString(ContentBrowserItem::ItemType type)
		{
			switch (type)
			{
			case ContentBrowserItem::ItemType::Asset: return "Asset";
			case ContentBrowserItem::ItemType::Directory: return "Directory";
			}

			return "Unknown";
		}
	}

}
