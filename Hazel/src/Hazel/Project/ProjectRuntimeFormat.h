#pragma once

#include <stdint.h>

#include "Hazel/Core/UUID.h"
#include "Hazel/Physics/PhysicsSettings.h"

namespace Hazel {

	struct ProjectInfo
	{
		struct FileHeader
		{
			const char HEADER[4] = { 'H','D','A','T' };
			uint32_t Version = 1;
		};

		struct Audio
		{
			double FileStreamingDurationThreshold;
		};

		FileHeader Header;
		AssetHandle StartScene;
		Audio AudioInfo;
		// Physics
	};

}
