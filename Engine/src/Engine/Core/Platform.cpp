#include "hzpch.h"
#include "Platform.h"

#include <ctime>
#include <chrono>

#include "spdlog/fmt/chrono.h"

namespace Hazel {

	uint64_t Platform::GetCurrentDateTimeU64()
	{
		std::string string = GetCurrentDateTimeString();
		return std::stoull(string);
	}

	std::string Platform::GetCurrentDateTimeString()
	{
		std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm* localTime = std::localtime(&currentTime);

		int year = localTime->tm_year + 1900;
		int month = localTime->tm_mon + 1;
		int day = localTime->tm_mday + 1;
		int hour = localTime->tm_hour;
		int minute = localTime->tm_min;

		//return fmt::format("{}{00}{00}{00}{00}", year, month, day, hour, minute);
		return fmt::format("{:%Y%m%d%H%M}", *localTime);
	}


}
