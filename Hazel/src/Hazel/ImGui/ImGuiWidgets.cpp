#include "hzpch.h"
#include "ImGuiWidgets.h"

#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Scene/Components.h"

namespace Hazel::UI
{
	bool Widgets::AssetSearchPopup(const char* ID, AssetType assetType, AssetHandle& selected, bool* cleared, const char* hint /*= "Search Assets"*/, ImVec2 size)
	{
		UI::ScopedColour popupBG(ImGuiCol_PopupBg, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.6f));

		bool modified = false;

		const auto& assetRegistry = Project::GetEditorAssetManager()->GetAssetRegistry();
		AssetHandle current = selected;

		ImGui::SetNextWindowSize({ size.x, 0.0f });

		static bool grabFocus = true;

		if (UI::BeginPopup(ID, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			static std::string searchString;

			if (ImGui::GetCurrentWindow()->Appearing)
			{
				grabFocus = true;
				searchString.clear();
			}

			// Search widget
			UI::ShiftCursor(3.0f, 2.0f);
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() * 2.0f);
			SearchWidget(searchString, hint, &grabFocus);

			const bool searching = !searchString.empty();

			// Clear property button
			if (cleared != nullptr)
			{
				UI::ScopedColourStack buttonColours(
					ImGuiCol_Button, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.0f),
					ImGuiCol_ButtonHovered, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.2f),
					ImGuiCol_ButtonActive, UI::ColourWithMultipliedValue(Colors::Theme::background, 0.9f));

				UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);

				ImGui::SetCursorPosX(0);

				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, searching);

				if (ImGui::Button("CLEAR", { ImGui::GetWindowWidth(), 0.0f }))
				{
					*cleared = true;
					modified = true;
				}

				ImGui::PopItemFlag();
			}

			// List of assets
			{
				UI::ScopedColour listBoxBg(ImGuiCol_FrameBg, IM_COL32_DISABLE);
				UI::ScopedColour listBoxBorder(ImGuiCol_Border, IM_COL32_DISABLE);

				ImGuiID listID = ImGui::GetID("##SearchListBox");
				if (ImGui::BeginListBox("##SearchListBox", ImVec2(-FLT_MIN, 0.0f)))
				{
					bool forwardFocus = false;

					ImGuiContext& g = *GImGui;
					if (g.NavJustMovedToId != 0)
					{
						if (g.NavJustMovedToId == listID)
						{
							forwardFocus = true;
							// ActivateItem moves keyboard navigation focuse inside of the window
							ImGui::ActivateItem(listID);
							ImGui::SetKeyboardFocusHere(1);
						}
					}

					for (const auto& [path, metadata] : assetRegistry)
					{
						if (metadata.Type != assetType)
							continue;

						if (metadata.IsMemoryAsset)
							continue;

						const std::string assetName = metadata.FilePath.stem().string();

						if (!searchString.empty() && !UI::IsMatchingSearch(assetName, searchString))
							continue;

						bool is_selected = (current == metadata.Handle);
						if (ImGui::Selectable(assetName.c_str(), is_selected))
						{
							current = metadata.Handle;
							selected = metadata.Handle;
							modified = true;
						}

						if (forwardFocus)
						{
							forwardFocus = false;
						}
						else if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndListBox();
				}
			}
			if (modified)
				ImGui::CloseCurrentPopup();

			UI::EndPopup();
		}

		return modified;
	}

	bool Widgets::AssetSearchPopup(const char* ID, AssetType assetType, AssetHandle& selected, bool allowMemoryOnlyAssets, bool* cleared, const char* hint /*= "Search Assets"*/, ImVec2 size)
	{
		UI::ScopedColour popupBG(ImGuiCol_PopupBg, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.6f));

		bool modified = false;

		const auto& assetRegistry = Project::GetEditorAssetManager()->GetAssetRegistry();
		AssetHandle current = selected;

		ImGui::SetNextWindowSize({ size.x, 0.0f });

		static bool grabFocus = true;

		if (UI::BeginPopup(ID, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			static std::string searchString;

			if (ImGui::GetCurrentWindow()->Appearing)
			{
				grabFocus = true;
				searchString.clear();
			}

			// Search widget
			UI::ShiftCursor(3.0f, 2.0f);
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() * 2.0f);
			SearchWidget(searchString, hint, &grabFocus);

			const bool searching = !searchString.empty();

			// Clear property button
			if (cleared != nullptr)
			{
				UI::ScopedColourStack buttonColours(
					ImGuiCol_Button, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.0f),
					ImGuiCol_ButtonHovered, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.2f),
					ImGuiCol_ButtonActive, UI::ColourWithMultipliedValue(Colors::Theme::background, 0.9f));

				UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);

				ImGui::SetCursorPosX(0);

				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, searching);

				if (ImGui::Button("CLEAR", { ImGui::GetWindowWidth(), 0.0f }))
				{
					*cleared = true;
					modified = true;
				}

				ImGui::PopItemFlag();
			}

			// List of assets
			{
				UI::ScopedColour listBoxBg(ImGuiCol_FrameBg, IM_COL32_DISABLE);
				UI::ScopedColour listBoxBorder(ImGuiCol_Border, IM_COL32_DISABLE);

				ImGuiID listID = ImGui::GetID("##SearchListBox");
				if (ImGui::BeginListBox("##SearchListBox", ImVec2(-FLT_MIN, 0.0f)))
				{
					bool forwardFocus = false;

					ImGuiContext& g = *GImGui;
					if (g.NavJustMovedToId != 0)
					{
						if (g.NavJustMovedToId == listID)
						{
							forwardFocus = true;
							// ActivateItem moves keyboard navigation focuse inside of the window
							ImGui::ActivateItem(listID);
							ImGui::SetKeyboardFocusHere(1);
						}
					}

					for (const auto& [path, metadata] : assetRegistry)
					{
						if (allowMemoryOnlyAssets != metadata.IsMemoryAsset)
							continue;

						if (metadata.Type != assetType)
							continue;

						const std::string assetName = metadata.IsMemoryAsset ? metadata.FilePath.string() : metadata.FilePath.stem().string();

						if (!searchString.empty() && !UI::IsMatchingSearch(assetName, searchString))
							continue;

						bool is_selected = (current == metadata.Handle);
						if (ImGui::Selectable(assetName.c_str(), is_selected))
						{
							current = metadata.Handle;
							selected = metadata.Handle;
							modified = true;
						}

						if (forwardFocus)
						{
							forwardFocus = false;
						}
						else if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndListBox();
				}
			}
			if (modified)
				ImGui::CloseCurrentPopup();

			UI::EndPopup();
		}

		return modified;
	}

	bool Widgets::AssetSearchPopup(const char* ID, AssetHandle& selected, bool* cleared, const char* hint, ImVec2 size, std::initializer_list<AssetType> assetTypes)
	{
		UI::ScopedColour popupBG(ImGuiCol_PopupBg, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.6f));

		bool modified = false;
		const bool AllowMemoryOnlyAssets = false;

		const auto& assetRegistry = Project::GetEditorAssetManager()->GetAssetRegistry();
		AssetHandle current = selected;

		ImGui::SetNextWindowSize({ size.x, 0.0f });

		static bool grabFocus = true;

		if (UI::BeginPopup(ID, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			static std::string searchString;

			if (ImGui::GetCurrentWindow()->Appearing)
			{
				grabFocus = true;
				searchString.clear();
			}

			// Search widget
			UI::ShiftCursor(3.0f, 2.0f);
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() * 2.0f);
			SearchWidget(searchString, hint, &grabFocus);

			const bool searching = !searchString.empty();

			// Clear property button
			if (cleared != nullptr)
			{
				UI::ScopedColourStack buttonColours(
					ImGuiCol_Button, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.0f),
					ImGuiCol_ButtonHovered, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.2f),
					ImGuiCol_ButtonActive, UI::ColourWithMultipliedValue(Colors::Theme::background, 0.9f));

				UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);

				ImGui::SetCursorPosX(0);

				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, searching);

				if (ImGui::Button("CLEAR", { ImGui::GetWindowWidth(), 0.0f }))
				{
					*cleared = true;
					modified = true;
				}

				ImGui::PopItemFlag();
			}

			// List of assets
			{
				UI::ScopedColour listBoxBg(ImGuiCol_FrameBg, IM_COL32_DISABLE);
				UI::ScopedColour listBoxBorder(ImGuiCol_Border, IM_COL32_DISABLE);

				ImGuiID listID = ImGui::GetID("##SearchListBox");
				if (ImGui::BeginListBox("##SearchListBox", ImVec2(-FLT_MIN, 0.0f)))
				{
					bool forwardFocus = false;

					ImGuiContext& g = *GImGui;
					if (g.NavJustMovedToId != 0)
					{
						if (g.NavJustMovedToId == listID)
						{
							forwardFocus = true;
							// ActivateItem moves keyboard navigation focuse inside of the window
							ImGui::ActivateItem(listID);
							ImGui::SetKeyboardFocusHere(1);
						}
					}

					for (const auto& [path, metadata] : assetRegistry)
					{
						bool isValidType = false;

						if (!AllowMemoryOnlyAssets && metadata.IsMemoryAsset)
							continue;

						for (AssetType type : assetTypes)
						{
							if (metadata.Type == type)
							{
								isValidType = true;
								break;
							}
						}

						if (!isValidType)
							continue;

						const std::string assetName = metadata.FilePath.stem().string();

						if (!searchString.empty() && !UI::IsMatchingSearch(assetName, searchString))
							continue;

						std::string label = fmt::format("{}##{}", assetName, metadata.FilePath.string());

						bool is_selected = (current == metadata.Handle);
						if (ImGui::Selectable(label.c_str(), is_selected))
						{
							current = metadata.Handle;
							selected = metadata.Handle;
							modified = true;
						}

						{
							const std::string& assetType = Utils::ToUpper(Utils::AssetTypeToString(metadata.Type));
							ImVec2 textSize = ImGui::CalcTextSize(assetType.c_str());
							ImVec2 rectSize = ImGui::GetItemRectSize();
							float paddingX = ImGui::GetStyle().FramePadding.x;

							ImGui::SameLine(rectSize.x - textSize.x - paddingX);

							UI::ScopedColour textColour(ImGuiCol_Text, Colors::Theme::textDarker);
							ImGui::TextUnformatted(assetType.c_str());
						}

						if (forwardFocus)
						{
							forwardFocus = false;
						}
						else if (is_selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndListBox();
				}
			}
			if (modified)
				ImGui::CloseCurrentPopup();

			UI::EndPopup();
		}

		return modified;
	}

	bool Widgets::EntitySearchPopup(const char* ID, Ref<Scene> scene, UUID& selected, bool* cleared /*= nullptr*/, const char* hint /*= "Search Entities"*/, ImVec2 size /*= { 250.0f, 350.0f }*/)
	{
		UI::ScopedColour popupBG(ImGuiCol_PopupBg, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.6f));

		bool modified = false;

		auto entities = scene->GetAllEntitiesWith<IDComponent, TagComponent>();
		UUID current = selected;

		ImGui::SetNextWindowSize({ size.x, 0.0f });

		static bool s_GrabFocus = true;

		if (UI::BeginPopup(ID, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
		{
			static std::string searchString;

			if (ImGui::GetCurrentWindow()->Appearing)
			{
				s_GrabFocus = true;
				searchString.clear();
			}

			// Search widget
			UI::ShiftCursor(3.0f, 2.0f);
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - ImGui::GetCursorPosX() * 2.0f);
			SearchWidget(searchString, hint, &s_GrabFocus);

			const bool searching = !searchString.empty();

			// Clear property button
			if (cleared != nullptr)
			{
				UI::ScopedColourStack buttonColours(
					ImGuiCol_Button, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.0f),
					ImGuiCol_ButtonHovered, UI::ColourWithMultipliedValue(Colors::Theme::background, 1.2f),
					ImGuiCol_ButtonActive, UI::ColourWithMultipliedValue(Colors::Theme::background, 0.9f));

				UI::ScopedStyle border(ImGuiStyleVar_FrameBorderSize, 0.0f);

				ImGui::SetCursorPosX(0);

				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, searching);

				if (ImGui::Button("CLEAR", { ImGui::GetWindowWidth(), 0.0f }))
				{
					*cleared = true;
					modified = true;
				}

				ImGui::PopItemFlag();
			}

			// List of entities
			{
				UI::ScopedColour listBoxBg(ImGuiCol_FrameBg, IM_COL32_DISABLE);
				UI::ScopedColour listBoxBorder(ImGuiCol_Border, IM_COL32_DISABLE);

				ImGuiID listID = ImGui::GetID("##SearchListBox");
				if (ImGui::BeginListBox("##SearchListBox", ImVec2(-FLT_MIN, 0.0f)))
				{
					bool forwardFocus = false;

					ImGuiContext& g = *GImGui;
					if (g.NavJustMovedToId != 0)
					{
						if (g.NavJustMovedToId == listID)
						{
							forwardFocus = true;
							// ActivateItem moves keyboard navigation focuse inside of the window
							ImGui::ActivateItem(listID);
							ImGui::SetKeyboardFocusHere(1);
						}
					}

					for (auto enttID : entities)
					{
						const auto& idComponent = entities.get<IDComponent>(enttID);
						const auto& tagComponent = entities.get<TagComponent>(enttID);

						if (!searchString.empty() && !UI::IsMatchingSearch(tagComponent.Tag, searchString))
							continue;

						bool is_selected = current == idComponent.ID;
						if (ImGui::Selectable(tagComponent.Tag.c_str(), is_selected))
						{
							current = selected = idComponent.ID;
							modified = true;
						}

						if (forwardFocus)
							forwardFocus = false;
						else if (is_selected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndListBox();
				}
			}
			if (modified)
				ImGui::CloseCurrentPopup();

			UI::EndPopup();
		}

		return modified;
	}

} // namespace Hazel::UI
