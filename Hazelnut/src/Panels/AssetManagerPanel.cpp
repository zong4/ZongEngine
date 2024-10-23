#include "AssetManagerPanel.h"

#include "Engine/ImGui/ImGui.h"
#include "Engine/Asset/AssetRegistry.h"
#include "Engine/Asset/AssetManager.h"

namespace Hazel {

	void AssetManagerPanel::OnImGuiRender(bool& isOpen)
	{
		if (ImGui::Begin("Asset Manager", &isOpen))
		{
			if (UI::BeginTreeNode("Registry"))
			{
				static char searchBuffer[256];
				UI::BeginPropertyGrid();
				static float regColumnWidth = 0.0f;
				if (regColumnWidth == 0.0f)
				{
					ImVec2 textSize = ImGui::CalcTextSize("Allow editing");
					regColumnWidth = textSize.x * 2.0f;
					ImGui::SetColumnWidth(0, regColumnWidth);
				}

				UI::Property("Search", searchBuffer, 256);

				const auto& assetRegistry = Project::GetEditorAssetManager()->GetAssetRegistry();

				static bool s_AllowEditing = false;
				UI::Property("Allow editing", s_AllowEditing);
				UI::SetTooltip("WARNING! This will directly edit Asset Registry UUIDs - only use this if you know what you're doing");
				UI::EndPropertyGrid();

				ImGui::BeginChild("RegistryEntries");
				{
					UI::BeginPropertyGrid();
					static float columnWidth = 0.0f;
					if (columnWidth == 0.0f)
					{
						ImVec2 textSize = ImGui::CalcTextSize("File Path");
						columnWidth = textSize.x * 2.0f;
						ImGui::SetColumnWidth(0, columnWidth);
					}

					int id = 0;
					for (const auto& [h, metadata] : assetRegistry)
					{
						ImGui::PushID(id++);

						std::string handle = fmt::format("{0}", metadata.Handle);
						std::string filepath = metadata.FilePath.string();
						std::string type = Utils::AssetTypeToString(metadata.Type);
						if (searchBuffer[0] != 0)
						{
							std::string searchString = searchBuffer;
							Utils::String::ToLower(searchString);
							if (Utils::String::ToLowerCopy(handle).find(searchString) != std::string::npos
								|| Utils::String::ToLowerCopy(filepath).find(searchString) != std::string::npos
								|| Utils::String::ToLowerCopy(type).find(searchString) != std::string::npos)
							{
								if (s_AllowEditing)
									UI::PropertyInput("Handle", (uint64_t&)metadata.Handle);
								else
									UI::Property("Handle", (const std::string&)fmt::format("{}", metadata.Handle));
								UI::Property("File Path", (const std::string&)filepath);
								UI::Property("Type", (const std::string&)type);
								UI::Separator();
							}
						}
						else
						{
							if (s_AllowEditing)
								UI::PropertyInput("Handle", (uint64_t&)metadata.Handle);
							else
								UI::Property("Handle", (const std::string&)fmt::format("{}", metadata.Handle));
							UI::Property("File Path", (const std::string&)metadata.FilePath.string());
							UI::Property("Type", (const std::string&)Utils::AssetTypeToString(metadata.Type));
							UI::Separator();
						}

						ImGui::PopID();
					}
					UI::EndPropertyGrid();
				}
				ImGui::EndChild();

				UI::EndTreeNode();
			}
		}

		ImGui::End();
	}

}
