#pragma once

#include "Engine/Core/Base.h"
#include "Log.h"

#ifdef ZONG_PLATFORM_WINDOWS
	#define ZONG_DEBUG_BREAK __debugbreak()
#elif defined(ZONG_COMPILER_CLANG)
	#define ZONG_DEBUG_BREAK __builtin_debugtrap()
#else
	#define ZONG_DEBUG_BREAK
#endif

#ifdef ZONG_DEBUG
	#define ZONG_ENABLE_ASSERTS
#endif

#define ZONG_ENABLE_VERIFY

#ifdef ZONG_ENABLE_ASSERTS
	#ifdef ZONG_COMPILER_CLANG
		#define ZONG_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Core, "Assertion Failed", ##__VA_ARGS__)
		#define ZONG_ASSERT_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Client, "Assertion Failed", ##__VA_ARGS__)
	#else
		#define ZONG_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Core, "Assertion Failed" __VA_OPT__(,) __VA_ARGS__)
		#define ZONG_ASSERT_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Client, "Assertion Failed" __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define ZONG_CORE_ASSERT(condition, ...) { if(!(condition)) { ZONG_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); ZONG_DEBUG_BREAK; } }
	#define ZONG_ASSERT(condition, ...) { if(!(condition)) { ZONG_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); ZONG_DEBUG_BREAK; } }
#else
	#define ZONG_CORE_ASSERT(condition, ...)
	#define ZONG_ASSERT(condition, ...)
#endif

#ifdef ZONG_ENABLE_VERIFY
	#ifdef ZONG_COMPILER_CLANG
		#define ZONG_CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Core, "Verify Failed", ##__VA_ARGS__)
		#define ZONG_VERIFY_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Client, "Verify Failed", ##__VA_ARGS__)
	#else
		#define ZONG_CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Core, "Verify Failed" __VA_OPT__(,) __VA_ARGS__)
		#define ZONG_VERIFY_MESSAGE_INTERNAL(...)  ::Hazel::Log::PrintAssertMessage(::Hazel::Log::Type::Client, "Verify Failed" __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define ZONG_CORE_VERIFY(condition, ...) { if(!(condition)) { ZONG_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); ZONG_DEBUG_BREAK; } }
	#define ZONG_VERIFY(condition, ...) { if(!(condition)) { ZONG_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); ZONG_DEBUG_BREAK; } }
#else
	#define ZONG_CORE_VERIFY(condition, ...)
	#define ZONG_VERIFY(condition, ...)
#endif
