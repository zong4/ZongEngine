#pragma once

#define HZ_ENABLE_PROFILING !HZ_DIST

#if HZ_ENABLE_PROFILING 
#include <tracy/Tracy.hpp>
#endif

#if HZ_ENABLE_PROFILING
#define HZ_PROFILE_MARK_FRAME			FrameMark;
// NOTE(Peter): Use HZ_PROFILE_FUNC ONLY at the top of a function
//				Use HZ_PROFILE_SCOPE / HZ_PROFILE_SCOPE_DYNAMIC for an inner scope
#define HZ_PROFILE_FUNC(...)			ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#define HZ_PROFILE_SCOPE(...)			HZ_PROFILE_FUNC(__VA_ARGS__)
#define HZ_PROFILE_SCOPE_DYNAMIC(NAME)  ZoneScoped; ZoneName(NAME, strlen(NAME))
#define HZ_PROFILE_THREAD(...)          tracy::SetThreadName(__VA_ARGS__)
#else
#define HZ_PROFILE_MARK_FRAME
#define HZ_PROFILE_FUNC(...)
#define HZ_PROFILE_SCOPE(...)
#define HZ_PROFILE_SCOPE_DYNAMIC(NAME)
#define HZ_PROFILE_THREAD(...)
#endif
