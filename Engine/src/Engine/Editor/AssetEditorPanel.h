#pragma once

#include "Engine/ImGui/ImGui.h"
#include "Engine/Core/TimeStep.h"
#include "Engine/Core/Events/Event.h"

namespace Engine {

	class AssetEditor
	{
	protected:
		AssetEditor(const char* id);

	public:
		virtual ~AssetEditor() {}

		virtual void OnUpdate(Timestep ts) {}
		virtual void OnEvent(Event& e) {}
		virtual void SetSceneContext(const Ref<Scene>& context) {}
		virtual void OnImGuiRender();
		void SetOpen(bool isOpen);
		virtual void SetAsset(const Ref<Asset>& asset) = 0;

		bool IsOpen() const { return m_IsOpen; }

		const std::string& GetTitle() const;
		void SetTitle(const std::string& newTitle);

	protected:
		void SetMinSize(uint32_t width, uint32_t height);
		void SetMaxSize(uint32_t width, uint32_t height);

		virtual ImGuiWindowFlags GetWindowFlags() { return 0; }

		// Subclass can optionally override these to customize window styling, e.g. window padding
		virtual void OnWindowStylePush() {}
		virtual void OnWindowStylePop() {}

	private:
		virtual void OnOpen() = 0;
		virtual void OnClose() = 0;
		virtual void Render() = 0;

	protected:
		std::string m_Id;          // Uniquely identifies the window independently of its title (e.g. for imgui.ini settings)
		std::string m_Title;
		std::string m_TitleAndId;  // Caches title###id to avoid computing it every frame
		ImVec2 m_MinSize, m_MaxSize;
		bool m_IsOpen = false;

	};

	class AssetEditorPanel
	{
	public:
		static void RegisterDefaultEditors();
		static void UnregisterAllEditors();
		static void OnUpdate(Timestep ts);
		static void OnEvent(Event& e);
		static void SetSceneContext(const Ref<Scene>& context);
		static void OnImGuiRender();
		static void OpenEditor(const Ref<Asset>& asset);

		template<typename T>
		static void RegisterEditor(AssetType type)
		{
			static_assert(std::is_base_of<AssetEditor, T>::value, "AssetEditorPanel::RegisterEditor requires template type to inherit from AssetEditor");
			ZONG_CORE_ASSERT(s_Editors.find(type) == s_Editors.end(), "There's already an editor for that asset!");
			s_Editors[type] = CreateScope<T>();
		}

	private:
		static std::unordered_map<AssetType, Scope<AssetEditor>> s_Editors;
		static Ref<Scene> s_SceneContext;
	};

}
