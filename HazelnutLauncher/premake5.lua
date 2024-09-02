project "HazelnutLauncher"
filter "not system:windows"
    kind "StaticLib"
filter "system:windows"
	kind "ConsoleApp"

	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

	links { "Hazel" }

	defines { "GLM_FORCE_DEPTH_ZERO_TO_ONE", }

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
		postbuildcommands { '{COPY} "../Hazel/vendor/NvidiaAftermath/lib/x64/windows/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"' }

	filter { "system:windows", "configurations:Debug" }
		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/windows/Debug/assimp-vc143-mtd.dll "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}

	filter { "system:windows", "configurations:Release or configurations:Dist" }
		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/windows/Release/assimp-vc143-mt.dll" "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/mono/bin/Release/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}

	filter { "system:windows", "configurations:Debug-AS" }	
		postbuildcommands {
			'{COPY} "../Hazel/vendor/assimp/bin/windows/Debug/assimp-vc143-mtd.dll "%{cfg.targetdir}"',
			'{COPY} "../Hazel/vendor/mono/bin/Debug/mono-2.0-sgen.dll" "%{cfg.targetdir}"'
		}

	filter "system:linux"
		defines { "HZ_PLATFORM_LINUX", "__EMULATE_UUID" }

		result, err = os.outputof("pkg-config --libs gtk+-3.0")
        linkoptions { result }

	filter "configurations:Debug or configurations:Debug-AS"
		symbols "On"
		defines { "HZ_DEBUG" }
		ProcessDependencies("Debug")

	filter { "system:windows", "configurations:Debug-AS" }
		sanitize { "Address" }
		flags { "NoRuntimeChecks", "NoIncrementalLink" }

	filter "configurations:Release"
		optimize "On"
		defines { "HZ_RELEASE", }
		ProcessDependencies("Release")

	filter { "configurations:Debug or configurations:Release" }
		defines {
			"HZ_TRACK_MEMORY",

			"JPH_DEBUG_RENDERER",
			"JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
			"JPH_EXTERNAL_PROFILE"
		}

	filter "configurations:Dist"
		optimize "On"
		defines { "HZ_DIST" }
		ProcessDependencies("Dist")
