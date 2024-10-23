#pragma once

#include "Engine/Core/Base.h"

#include <string>
#include <ctime>

namespace Hazel {

	enum class ConsoleMessageFlags : int16_t
	{
		None = -1,
		Info = BIT(0),
		Warning = BIT(1),
		Error = BIT(2),

		All = Info | Warning | Error
	};

	struct ConsoleMessage
	{
		std::string ShortMessage;
		std::string LongMessage;
		int16_t Flags;

		time_t Time;
	};
	
}
