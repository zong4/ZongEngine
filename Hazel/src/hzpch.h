#pragma once

#ifdef HZ_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <filesystem>
#include <thread>

#include <Hazel/Core/Version.h>
#include <Hazel/Core/Assert.h>
#include <Hazel/Core/Base.h>
#include <Hazel/Core/Events/Event.h>
#include <Hazel/Core/Log.h>
#include <Hazel/Core/Math/Mat4.h>
#include <Hazel/Core/Memory.h>
#include <Hazel/Core/Delegate.h>

// Jolt (Safety because this file has to be included before all other Jolt headers, at all times)
#ifdef HZ_DEBUG // NOTE(Emily): This is a bit of a hacky fix for some dark magic that happens in Jolt
				// 				We'll need to address this in future.
#define JPH_ENABLE_ASSERTS
#endif
#include <Jolt/Jolt.h>
