project "Tracy"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "Off"

	--toolset "clang"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"tracy/public/client/**.h",
		"tracy/public/client/**.hpp",
		"tracy/public/client/**.cpp",

		"tracy/public/common/**.h",
		"tracy/public/common/**.hpp",
		"tracy/public/common/**.cpp",

		"tracy/public/tracy/**.h",
		"tracy/public/tracy/**.hpp",
		"tracy/public/tracy/**.cpp",

		"tracy/public/libbacktrace/alloc.cpp",
		"tracy/public/libbacktrace/sort.cpp",
		"tracy/public/libbacktrace/state.cpp",
	}

	includedirs { "tracy/public/" }

	filter "system:windows"
		systemversion "latest"

	filter "system:linux"
		files {
			"tracy/public/libbacktrace/posix.cpp",
			"tracy/public/libbacktrace/mmapio.cpp",
			"tracy/public/libbacktrace/macho.cpp",
			"tracy/public/libbacktrace/fileline.cpp",
			"tracy/public/libbacktrace/elf.cpp",
			"tracy/public/libbacktrace/dwarf.cpp",
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"
		conformancemode "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"
		conformancemode "On"

		defines {
			"TRACY_ENABLE",
			"TRACY_ON_DEMAND"
		}

	filter "configurations:Dist"
		runtime "Release"
		optimize "Full"
		conformancemode "On"

		defines {
			"TRACY_ENABLE",
			"TRACY_ON_DEMAND"
		}