#pragma once

#include <string>

namespace Engine {

	namespace Tiering {
	
		namespace Renderer {

			enum class ShadowQualitySetting
			{
				None = 0, Low = 1, High = 2
			};

			enum class ShadowResolutionSetting
			{
				None = 0, Low = 1, Medium = 2, High = 3
			};

			enum class AmbientOcclusionTypeSetting
			{
				None = 0, GTAO = 1
			};

			enum class AmbientOcclusionQualitySetting
			{
				None = 0, High = 1, Ultra = 2
			};

			enum class SSRQualitySetting
			{
				Off = 0, Medium = 1, High = 2
			};

			struct RendererTieringSettings
			{
				float RendererScale = 1.0f;
				bool Windowed = false;
				bool VSync = true;

				// Shadows
				bool EnableShadows = true;
				Renderer::ShadowQualitySetting ShadowQuality = Renderer::ShadowQualitySetting::High;
				Renderer::ShadowResolutionSetting ShadowResolution = Renderer::ShadowResolutionSetting::High;

				// Ambient Occlusion
				bool EnableAO = true;
				Renderer::AmbientOcclusionTypeSetting AOType = Renderer::AmbientOcclusionTypeSetting::GTAO;
				Renderer::AmbientOcclusionQualitySetting AOQuality = Renderer::AmbientOcclusionQualitySetting::Ultra;
				Renderer::SSRQualitySetting SSRQuality = Renderer::SSRQualitySetting::Off;

				bool EnableBloom = true;
			};

			namespace Utils {

				inline const char* ShadowQualitySettingToString(ShadowQualitySetting shadowQualitySetting)
				{
					switch (shadowQualitySetting)
					{
						case ShadowQualitySetting::None: return "None";
						case ShadowQualitySetting::Low:  return "Low";
						case ShadowQualitySetting::High: return "High";
					}

					return nullptr;
				}

				inline ShadowQualitySetting ShadowQualitySettingFromString(std::string_view shadowQualitySetting)
				{
					if (shadowQualitySetting == "None") return ShadowQualitySetting::None;
					if (shadowQualitySetting == "Low")  return ShadowQualitySetting::Low;
					if (shadowQualitySetting == "High") return ShadowQualitySetting::High;

					return ShadowQualitySetting::None;
				}

				inline const char* ShadowResolutionSettingToString(ShadowResolutionSetting shadowResolutionSetting)
				{
					switch (shadowResolutionSetting)
					{
						case ShadowResolutionSetting::None:   return "None";
						case ShadowResolutionSetting::Low:    return "Low";
						case ShadowResolutionSetting::Medium: return "Medium";
						case ShadowResolutionSetting::High:   return "High";
					}

					return nullptr;
				}

				inline ShadowResolutionSetting ShadowResolutionSettingFromString(std::string_view shadowResolutionSetting)
				{
					if (shadowResolutionSetting == "None")   return ShadowResolutionSetting::None;
					if (shadowResolutionSetting == "Low")    return ShadowResolutionSetting::Low;
					if (shadowResolutionSetting == "Medium") return ShadowResolutionSetting::Medium;
					if (shadowResolutionSetting == "High")   return ShadowResolutionSetting::High;

					return ShadowResolutionSetting::None;
				}

				inline const char* AmbientOcclusionQualitySettingToString(AmbientOcclusionQualitySetting ambientOcclusionQualitySetting)
				{
					switch (ambientOcclusionQualitySetting)
					{
						case AmbientOcclusionQualitySetting::None:   return "None";
						case AmbientOcclusionQualitySetting::High:   return "High";
						case AmbientOcclusionQualitySetting::Ultra:  return "Ultra";
					}

					return nullptr;
				}

				inline AmbientOcclusionQualitySetting AmbientOcclusionQualitySettingFromString(std::string_view ambientOcclusionQualitySetting)
				{
					if (ambientOcclusionQualitySetting == "None")   return AmbientOcclusionQualitySetting::None;
					if (ambientOcclusionQualitySetting == "Low")    return AmbientOcclusionQualitySetting::High; // NOTE(Yan): low has been renamed to high, currently there is no low
					if (ambientOcclusionQualitySetting == "High")   return AmbientOcclusionQualitySetting::High;
					if (ambientOcclusionQualitySetting == "Ultra")  return AmbientOcclusionQualitySetting::Ultra;

					return AmbientOcclusionQualitySetting::None;
				}

				inline const char* SSRQualitySettingToString(SSRQualitySetting ssrQualitySetting)
				{
					switch (ssrQualitySetting)
					{
						case SSRQualitySetting::Off:    return "Off";
						case SSRQualitySetting::Medium: return "Medium";
						case SSRQualitySetting::High:   return "High";
					}

					return nullptr;
				}

				inline SSRQualitySetting SSRQualitySettingFromString(std::string_view ssrQualitySetting)
				{
					if (ssrQualitySetting == "Off")    return SSRQualitySetting::Off;
					if (ssrQualitySetting == "Medium") return SSRQualitySetting::Medium;
					if (ssrQualitySetting == "High")   return SSRQualitySetting::High;

					return SSRQualitySetting::Off;
				}

			}
		}

		struct TieringSettings
		{
			Renderer::RendererTieringSettings RendererTS;
		};

	}

}
