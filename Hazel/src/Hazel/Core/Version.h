#pragma once

#define HZ_VERSION "2024.1.1"

//
// Build Configuration
//
#if defined(HZ_DEBUG)
	#define HZ_BUILD_CONFIG_NAME "Debug"
#elif defined(HZ_RELEASE)
	#define HZ_BUILD_CONFIG_NAME "Release"
#elif defined(HZ_DIST)
	#define HZ_BUILD_CONFIG_NAME "Dist"
#else
	#error Undefined configuration?
#endif

//
// Build Platform
//
#if defined(HZ_PLATFORM_WINDOWS)
	#define HZ_BUILD_PLATFORM_NAME "Windows x64"
#elif defined(HZ_PLATFORM_LINUX)
	#define HZ_BUILD_PLATFORM_NAME "Linux"
#else
	#define HZ_BUILD_PLATFORM_NAME "Unknown"
#endif

#define HZ_VERSION_LONG "Hazel " HZ_VERSION " (" HZ_BUILD_PLATFORM_NAME " " HZ_BUILD_CONFIG_NAME ")"
