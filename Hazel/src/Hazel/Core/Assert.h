#pragma once

#include "Base.h"
#include "Log.h"

#ifdef HZ_PLATFORM_WINDOWS
	#define HZ_DEBUG_BREAK __debugbreak()
#elif defined(HZ_COMPILER_CLANG)
	#define HZ_DEBUG_BREAK __builtin_debugtrap()
#else
	#define HZ_DEBUG_BREAK
#endif

#ifdef HZ_DEBUG
	#define HZ_ENABLE_ASSERTS
#endif

#define HZ_ENABLE_VERIFY

#ifdef HZ_ENABLE_ASSERTS
	#ifdef HZ_COMPILER_CLANG
		#define HZ_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Core, "Assertion Failed", ##__VA_ARGS__)
		#define HZ_ASSERT_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Client, "Assertion Failed", ##__VA_ARGS__)
	#else
		#define HZ_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Core, "Assertion Failed" __VA_OPT__(,) __VA_ARGS__)
		#define HZ_ASSERT_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Client, "Assertion Failed" __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define HZ_CORE_ASSERT(condition, ...) { if(!(condition)) { HZ_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); HZ_DEBUG_BREAK; } }
	#define HZ_ASSERT(condition, ...) { if(!(condition)) { HZ_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); HZ_DEBUG_BREAK; } }
#else
	#define HZ_CORE_ASSERT(condition, ...)
	#define HZ_ASSERT(condition, ...)
#endif

#ifdef HZ_ENABLE_VERIFY
	#ifdef HZ_COMPILER_CLANG
		#define HZ_CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Core, "Verify Failed", ##__VA_ARGS__)
		#define HZ_VERIFY_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Client, "Verify Failed", ##__VA_ARGS__)
	#else
		#define HZ_CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Core, "Verify Failed" __VA_OPT__(,) __VA_ARGS__)
		#define HZ_VERIFY_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Client, "Verify Failed" __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define HZ_CORE_VERIFY(condition, ...) { if(!(condition)) { HZ_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); HZ_DEBUG_BREAK; } }
	#define HZ_VERIFY(condition, ...) { if(!(condition)) { HZ_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); HZ_DEBUG_BREAK; } }
#else
	#define HZ_CORE_VERIFY(condition, ...)
	#define HZ_VERIFY(condition, ...)
#endif
