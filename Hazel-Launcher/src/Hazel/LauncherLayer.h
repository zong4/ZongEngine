#pragma once

#include "Walnut/Layer.h"
#include "Walnut/Image.h"

#include "Hazel/Project/TieringSettings.h"

#include <memory>
#include <set>
#include <filesystem>

namespace Hazel {

	struct Resolution
	{
		int Width;
		int Height;
		int RefreshRate;

		bool operator<(const Resolution& other) const
		{
			return Width > other.Width || Height > other.Height || RefreshRate > other.RefreshRate;
		}
	};

	struct LauncherSpecification
	{
		std::string AppExecutableTitle;
		std::filesystem::path AppExecutablePath;
		std::filesystem::path TieringSettingsPath;
		std::filesystem::path AppSettingsPath;
	};

	class LauncherLayer : public Walnut::Layer
	{
	public:
		LauncherLayer(const LauncherSpecification& specification);

		virtual void OnAttach() override;
		virtual void OnUIRender() override;

		bool IsTitleBarHovered() const { return m_TitleBarHovered; }
	private:
		float UI_DrawTitlebar();
	private:
		LauncherSpecification m_Specification;

		std::shared_ptr<Walnut::Image> m_CoverImage;
		std::shared_ptr<Walnut::Image> m_LogoTex;

		std::shared_ptr<Walnut::Image> m_IconMinimize;
		std::shared_ptr<Walnut::Image> m_IconClose;
		std::set<Resolution> m_Resolutions;

		bool m_TitleBarHovered = false;
		bool m_ShowErrorDialog = false;

		int m_MouseSensitivity = 5;

		Tiering::TieringSettings m_TieringSettings;
	};

}
