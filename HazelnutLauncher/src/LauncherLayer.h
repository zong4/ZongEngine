#pragma once

#include <Hazel/Core/Layer.h>
#include <Hazel/Core/Ref.h>
#include <Hazel/Project/UserPreferences.h>
#include <Hazel/Renderer/Texture.h>

#include <imgui/imgui.h>

#include <filesystem>
#include <functional>

namespace Hazel {

	struct LauncherProperties
	{
		Ref<UserPreferences> UserPreferences;
		std::string InstallPath;
		std::function<void(std::string)> ProjectOpenedCallback;
	};

	class LauncherLayer : public Layer
	{
	public:
		LauncherLayer(const LauncherProperties& properties);
		virtual ~LauncherLayer() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnImGuiRender() override;

	private:
		void CreateProject(std::filesystem::path projectPath);
		void AddProjectToRecents(const std::filesystem::path& projectFile);

	private:
		LauncherProperties m_Properties;
		ImGuiID m_HoveredProjectID;
		Ref<Texture2D> m_HazelLogoTexture;
	};

}
