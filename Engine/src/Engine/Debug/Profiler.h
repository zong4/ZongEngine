#pragma once

#define ZONG_ENABLE_PROFILING !ZONG_DIST

#if ZONG_ENABLE_PROFILING 
#include <tracy/Tracy.hpp>
#endif

#if ZONG_ENABLE_PROFILING
#define ZONG_PROFILE_MARK_FRAME			FrameMark;
// NOTE(Peter): Use ZONG_PROFILE_FUNC ONLY at the top of a function
//				Use ZONG_PROFILE_SCOPE / ZONG_PROFILE_SCOPE_DYNAMIC for an inner scope
#define ZONG_PROFILE_FUNC(...)			ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#define ZONG_PROFILE_SCOPE(...)			ZONG_PROFILE_FUNC(__VA_ARGS__)
#define ZONG_PROFILE_SCOPE_DYNAMIC(NAME)  ZoneScoped; ZoneName(NAME, strlen(NAME))
#define ZONG_PROFILE_THREAD(...)          tracy::SetThreadName(__VA_ARGS__)
#else
#define ZONG_PROFILE_MARK_FRAME
#define ZONG_PROFILE_FUNC(...)
#define ZONG_PROFILE_SCOPE(...)
#define ZONG_PROFILE_SCOPE_DYNAMIC(NAME)
#define ZONG_PROFILE_THREAD(...)
#endif
