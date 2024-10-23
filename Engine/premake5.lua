project "Engine"
	kind "StaticLib"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "pch.h"
	pchsource "src/pch.cpp"

	files {
		"src/**.h",
		"src/**.c",
		"src/**.hpp",
		"src/**.cpp",

		"Platform/" .. firstToUpper(os.target()) .. "/**.hpp",
		"Platform/" .. firstToUpper(os.target()) .. "/**.cpp",

		"vendor/FastNoise/**.cpp",

		"vendor/yaml-cpp/src/**.cpp",
		"vendor/yaml-cpp/src/**.h",
		"vendor/yaml-cpp/include/**.h",
		"vendor/VulkanMemoryAllocator/**.h",
		"vendor/VulkanMemoryAllocator/**.cpp",

		"vendor/imgui/misc/cpp/imgui_stdlib.cpp",
		"vendor/imgui/misc/cpp/imgui_stdlib.h"
	}

	includedirs { "src/", "vendor/", }

	IncludeDependencies()

	defines { "GLM_FORCE_DEPTH_ZERO_TO_ONE", }

	filter "files:vendor/FastNoise/**.cpp or files:vendor/yaml-cpp/src/**.cpp or files:vendor/imgui/misc/cpp/imgui_stdlib.cpp or files:src/Engine/Tiering/TieringSerializer.cpp or files:src/Engine/Core/ApplicationSettings.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		systemversion "latest"
		defines { "ZONG_PLATFORM_WINDOWS", }

	filter "system:linux"
		defines { "ZONG_PLATFORM_LINUX", "__EMULATE_UUID" }

	filter "configurations:Debug or configurations:Debug-AS"
		symbols "On"
		defines { "ZONG_DEBUG", "_DEBUG", }

	filter { "system:windows", "configurations:Debug-AS" }	
		sanitize { "Address" }
		flags { "NoRuntimeChecks", "NoIncrementalLink" }

	filter "configurations:Release"
		optimize "On"
		vectorextensions "AVX2"
		isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }
		defines { "ZONG_RELEASE", "NDEBUG", }

	filter { "configurations:Debug or configurations:Debug-AS or configurations:Release" }
		defines {
			"ZONG_TRACK_MEMORY",

			"JPH_DEBUG_RENDERER",
			"JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
			"JPH_EXTERNAL_PROFILE"
		}

	filter "configurations:Dist"
		optimize "On"
		symbols "Off"
		vectorextensions "AVX2"
		isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }
		defines { "ZONG_DIST" }

		removefiles {
			"src/Engine/Platform/Vulkan/ShaderCompiler/**.cpp",
			"src/Engine/Platform/Vulkan/Debug/**.cpp",

			"src/Engine/Asset/AssimpAnimationImporter.cpp",
			"src/Engine/Asset/AssimpMeshImporter.cpp",
		}
	