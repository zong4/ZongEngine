project "Hazel-Runtime"
	kind "ConsoleApp"
	targetname "Hazel-Runtime"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	links  { "Hazel" }

	defines  { "GLM_FORCE_DEPTH_ZERO_TO_ONE", }

	files {
		"src/**.h", 
		"src/**.c", 
		"src/**.hpp", 
		"src/**.cpp" 
	}

	includedirs {
		"src/",
		"../Hazel/src",
		"../Hazel/vendor",
	}

	filter "system:windows"
		systemversion "latest"
		defines { "HZ_PLATFORM_WINDOWS" }

		-- Visual Studio build metadata
		files {
			"Hazel-Runtime.aps",
			"Hazel-Runtime.rc",
			"resource.h"
		}

	filter { "system:windows", "configurations:Debug" }
		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/windows/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/NvidiaAftermath/lib/x64/windows/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"'
		}

	filter { "system:windows", "configurations:Debug-AS" }
		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/windows/Debug/assimp-vc143-mtd.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/NvidiaAftermath/lib/x64/windows/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"'
		}

	filter { "system:windows", "configurations:Release" }
		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/windows/Release/assimp-vc143-mt.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/NvidiaAftermath/lib/x64/windows/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"'
		}

	filter "system:linux"
		defines { "HZ_PLATFORM_LINUX", "__EMULATE_UUID" }

		links {
			"pthread"
		}

		result, err = os.outputof("pkg-config --libs gtk+-3.0")
		linkoptions {
			result,
			"-pthread",
			"-ldl"
		}

	filter "configurations:Debug or configurations:Debug-AS"
		symbols "On"
		defines { "HZ_DEBUG", }

		ProcessDependencies("Debug")

	filter { "system:windows", "configurations:Debug-AS" }
		sanitize { "Address" }
		flags { "NoRuntimeChecks", "NoIncrementalLink" }

	filter "configurations:Release"
		optimize "On"
        vectorextensions "AVX2"
        isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }
		defines { "HZ_RELEASE", "HZ_TRACK_MEMORY", }

		ProcessDependencies("Release")

	filter "configurations:Debug or configurations:Release"
		defines {
			"HZ_TRACK_MEMORY",
			
            "JPH_DEBUG_RENDERER",
            "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
            "JPH_EXTERNAL_PROFILE"
		}

	filter "configurations:Dist"
		kind "WindowedApp"
		optimize "On"
		symbols "Off"
        vectorextensions "AVX2"
        isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }
		defines { "HZ_DIST" }

		ProcessDependencies("Dist")

        