#pragma once

#include <string>

namespace Hazel {

	class Platform
	{
	public:
		static uint64_t GetCurrentDateTimeU64();
		static std::string GetCurrentDateTimeString();

	};

}
