#pragma once

#include <string>

namespace Hazel {

	struct RendererConfig
	{
		uint32_t FramesInFlight = 3;

		bool ComputeEnvironmentMaps = true;

		// Tiering settings
		uint32_t EnvironmentMapResolution = 1024;
		uint32_t IrradianceMapComputeSamples = 512;

		std::string ShaderPackPath;
	};

}
