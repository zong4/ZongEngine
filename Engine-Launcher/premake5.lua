project "Engine-Launcher"
filter "not system:windows"
    kind "StaticLib"
filter "system:windows"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"
	targetname "SavingCaptainCoffeeLauncher"
	
	targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
	objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")
	
	defines 
	{
		"GLM_FORCE_DEPTH_ZERO_TO_ONE"
	}
	
	files 
	{ 
		"src/**.h", 
		"src/**.c", 
		"src/**.hpp", 
		"src/**.cpp",

		"../Engine/vendor/yaml-cpp/src/**.cpp",
		"../Engine/vendor/yaml-cpp/src/**.h",
		"../Engine/vendor/yaml-cpp/include/**.h",

		-- Include tiering serialization
		"../Engine/src/Engine/Tiering/TieringSerializer.h",
		"../Engine/src/Engine/Tiering/TieringSerializer.cpp",
		"../Engine/src/Engine/Core/ApplicationSettings.h",
		"../Engine/src/Engine/Core/ApplicationSettings.cpp",
	}
	
	includedirs 
	{
		"src",
		"../Engine/vendor",
		"../Engine/src",
		"../Engine/vendor/yaml-cpp/include/",
		"%{Dependencies.ImGui.IncludeDir}",
		"%{Dependencies.Vulkan.Windows.IncludeDir}",
		"%{Dependencies.GLFW.IncludeDir}",
		"%{Dependencies.STB.IncludeDir}",
		"%{Dependencies.GLM.IncludeDir}",
	}

	links
	{ 
		"GLFW",
		"ImGui",
		
		"%{Dependencies.Vulkan.Windows.LibDir}%{Dependencies.Vulkan.Windows.LibName}"
	}
	
	filter "system:windows"
		systemversion "latest"
	
		defines 
		{ 
			"ZONG_PLATFORM_WINDOWS",
			"WL_PLATFORM_WINDOWS"
		}
	
	filter "configurations:Debug or configurations:Debug-AS"
		symbols "on"
	
		defines 
		{
			"ZONG_DEBUG",
			"WL_DEBUG"
		}
	
	filter "configurations:Release"
		optimize "On"
        vectorextensions "AVX2"
        isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }
	
		defines 
		{
			"ZONG_RELEASE",
			"WL_RELEASE"
		}
	
	filter "configurations:Dist"
		kind "WindowedApp"
		optimize "On"
		symbols "Off"
		vectorextensions "AVX2"
		isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }
		defines { "ZONG_DIST" }

		defines 
		{
			"ZONG_DIST",
			"WL_DIST"
		}