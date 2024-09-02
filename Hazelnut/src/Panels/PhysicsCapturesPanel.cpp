#include "PhysicsCapturesPanel.h"

#include "Hazel/Physics/PhysicsSystem.h"

#include "Hazel/ImGui/ImGui.h"

namespace Hazel {

	void PhysicsCapturesPanel::OnImGuiRender(bool& isOpen)
	{
		std::filesystem::path captureToDelete = "";

		if (ImGui::Begin("Physics Captures", &isOpen))
		{
			Ref<PhysicsCaptureManager> captureManager = PhysicsSystem::GetAPI()->GetCaptureManager();
			const auto& captures = captureManager->GetAllCaptures();

			static char searchBuffer[256] = { 0 };

			{
				UI::ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));

				float contentWidth = ImGui::GetContentRegionAvail().x;
				float buttonWidth = contentWidth * 0.2f;
				float textFieldWidth = contentWidth - buttonWidth;

				ImGui::SetNextItemWidth(textFieldWidth);
				ImGui::InputTextWithHint("##capturesSearch", "Search...", searchBuffer, 256);
				ImGui::SameLine();

				if (ImGui::Button("Clear Captures", ImVec2(buttonWidth, 0.0f)))
				{
					for (const std::filesystem::path& capturePath : captures)
						FileSystem::DeleteFile(capturePath);
					PhysicsSystem::GetAPI()->GetCaptureManager()->ClearCaptures();
				}
			}

			// Iterate in reverse order because the captures are stored oldest -> newest
			for (size_t i = captures.size(); i-- > 0; )
			{
				std::string capturePath = captures[i].string();

				UI::ScopedStyle framePadding(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 8.0f));
				float contentWidth = ImGui::GetContentRegionAvail().x;
				float buttonWidth = contentWidth * 0.1f;
				float textFieldWidth = contentWidth - buttonWidth * 2.0f;

				if (searchBuffer[0] != 0)
				{
					std::string searchString = searchBuffer;
					Utils::String::ToLower(searchString);
					if (Utils::String::ToLowerCopy(capturePath).find(searchString) != std::string::npos)
					{
						UI::BeginDisabled();
						ImGui::SetNextItemWidth(textFieldWidth);
						ImGui::InputText(fmt::format("##captureFilePath-{}", capturePath).c_str(), capturePath.data(), capturePath.size(), ImGuiInputTextFlags_ReadOnly);
						UI::EndDisabled();

						ImGui::SameLine();
						if (ImGui::Button(fmt::format("Open##{}", capturePath).c_str(), ImVec2(buttonWidth, 0.0f)))
							captureManager->OpenCapture(capturePath);
						ImGui::SameLine();
						if (ImGui::Button(fmt::format("Delete##{}", capturePath).c_str(), ImVec2(buttonWidth, 0.0f)))
							captureToDelete = capturePath;
						UI::Separator();
					}
				}
				else
				{
					UI::BeginDisabled();
					ImGui::SetNextItemWidth(textFieldWidth);
					ImGui::InputText(fmt::format("##captureFilePath-{}", capturePath).c_str(), capturePath.data(), capturePath.size(), ImGuiInputTextFlags_ReadOnly);
					UI::EndDisabled();

					ImGui::SameLine();
					if (ImGui::Button(fmt::format("Open##{}", capturePath).c_str(), ImVec2(buttonWidth, 0.0f)))
						captureManager->OpenCapture(capturePath);

					ImGui::SameLine();
					if (ImGui::Button(fmt::format("Delete##{}", capturePath).c_str(), ImVec2(buttonWidth, 0.0f)))
						captureToDelete = capturePath;

					UI::Separator();
				}
			}
		}

		ImGui::End();

		if (!captureToDelete.empty())
		{
			FileSystem::DeleteFile(captureToDelete);
			PhysicsSystem::GetAPI()->GetCaptureManager()->RemoveCapture(captureToDelete);
		}
	}

}
