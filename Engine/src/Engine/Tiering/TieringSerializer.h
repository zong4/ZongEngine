#pragma once

#include "Engine/Project/TieringSettings.h"

#include <filesystem>

namespace Engine {

	class TieringSerializer
	{
	public:
		static void Serialize(const Tiering::TieringSettings& tieringSettings, const std::filesystem::path& filepath);
		static bool Deserialize(Tiering::TieringSettings& outTieringSettings, const std::filesystem::path& filepath);
	};

}
