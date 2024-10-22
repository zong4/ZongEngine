#pragma once

#include "Hazel/Core/Events/KeyEvent.h"
#include "Hazel/Audio/Editor/OrderedVector.h"
#include "TreeModel.h"

#include "Hazel/Audio/AudioEvents/CommandID.h"

#include "Hazel/Editor/EditorPanel.h"

struct ImGuiWindow;

namespace Hazel
{
	class Texture2D;
}
namespace YAML
{
	class Emitter;
	class Node;
}

namespace Hazel
{
	namespace Audio
	{
		struct TriggerCommand;
	}

	class AudioEventsEditor : public EditorPanel
	{
	public:
		using CommandsTree = TreeModel<Audio::CommandID>;

		AudioEventsEditor();
		virtual ~AudioEventsEditor();

		virtual void OnProjectChanged(const Ref<Project>& project) override;
		virtual void OnImGuiRender(bool& isOpen) override;
		virtual void OnEvent(Event& e) override;

		static void Serialize();
		static void SerializeTree(YAML::Emitter& out);
		static void DeserializeTree(std::vector<YAML::Node>& data);

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& e);

		void OnOpennesChange(bool opened);

		void DrawEventsList();
		void DrawEventDetails();
		void DrawFolderDetails();
		void DrawSelectionDetails();

		void DrawTreeNode(CommandsTree::Node* treeNode, bool isRootNode = false);

		void OnSelectionChanged(Audio::CommandID commandID);
		void OnDeleteNode(CommandsTree::Node* treeNode);

	private:
		std::string GetUniqueName();
		std::string GetUniqueFolderName(CommandsTree::Node* parentNode, const std::string& nameBase);

		void ActivateRename(CommandsTree::Node* treeNode);
		void OnEntryRenamed(CommandsTree::Node* treeNode, const char* newName);
		bool IsRenaming();
		std::optional<std::string> CheckNameConflict(const std::string& name, CommandsTree::Node* parentNode,
															std::function<void()> onReplaceDestination, std::function<void()> onCancel = nullptr);
		bool CheckAndResolveNameConflict(CommandsTree::Node* newParent, CommandsTree::Node* node, CommandsTree::Node* oldParent);

		void OnRegistryChanged();

		void Reset();

	private:
		bool m_Open = false;
		bool m_IsWindowFocused = false;

		WeakRef<AudioCommandRegistry> m_Registry;
		// Used to track when project changes and we need to serialize Tree View
		static inline std::filesystem::path s_CurrentRegistryPath;

		CommandsTree::Node* m_NewlyCreatedNode = nullptr;
		CommandsTree::Node* m_RenamingEntry = nullptr;
		OrderedVector<CommandsTree::Node*> m_Selection;

		std::vector<CommandsTree::Node*> m_MarkedForDelete;

		struct ReorderData
		{
			Ref<CommandsTree::Node> Node;
			CommandsTree::Node* NewParent;
		};
		static std::vector<ReorderData> s_MarkedForReorder;

		static CommandsTree s_Tree;

		static ImGuiWindow* s_WindowHandle;

		// Flags
		bool m_Renaming = false;
		std::string m_RenameConflict;
		bool m_RenamingActive = false;
	};


} // namespace Hazel
