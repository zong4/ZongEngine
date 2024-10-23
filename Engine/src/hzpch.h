#pragma once

#ifdef HZ_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <filesystem>

#include <Engine/Core/Assert.h>
#include <Engine/Core/Base.h>
#include <Engine/Core/Events/Event.h>
#include <Engine/Core/Log.h>
#include <Engine/Core/Math/Mat4.h>
#include <Engine/Core/Memory.h>
#include <Engine/Core/Delegate.h>

// Jolt (Safety because this file has to be included before all other Jolt headers, at all times)
#ifdef HZ_DEBUG // NOTE(Emily): This is a bit of a hacky fix for some dark magic that happens in Jolt
				// 				We'll need to address this in future.
#define JPH_ENABLE_ASSERTS
#endif
#include <Jolt/Jolt.h>
