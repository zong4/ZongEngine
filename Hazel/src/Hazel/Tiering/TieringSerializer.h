#pragma once

#include "Hazel/Project/TieringSettings.h"

#include <filesystem>

namespace Hazel {

	class TieringSerializer
	{
	public:
		static void Serialize(const Tiering::TieringSettings& tieringSettings, const std::filesystem::path& filepath);
		static bool Deserialize(Tiering::TieringSettings& outTieringSettings, const std::filesystem::path& filepath);
	};

}
