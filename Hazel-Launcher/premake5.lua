project "Hazel-Launcher"
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

		"../Hazel/vendor/yaml-cpp/src/**.cpp",
		"../Hazel/vendor/yaml-cpp/src/**.h",
		"../Hazel/vendor/yaml-cpp/include/**.h",

		-- Include tiering serialization
		"../Hazel/src/Hazel/Tiering/TieringSerializer.h",
		"../Hazel/src/Hazel/Tiering/TieringSerializer.cpp",
		"../Hazel/src/Hazel/Core/ApplicationSettings.h",
		"../Hazel/src/Hazel/Core/ApplicationSettings.cpp",
	}
	
	includedirs 
	{
		"src",
		"../Hazel/vendor",
		"../Hazel/src",
		"../Hazel/vendor/yaml-cpp/include/",
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
			"HZ_PLATFORM_WINDOWS",
			"WL_PLATFORM_WINDOWS"
		}
	
	filter "configurations:Debug or configurations:Debug-AS"
		symbols "on"
	
		defines 
		{
			"HZ_DEBUG",
			"WL_DEBUG"
		}
	
	filter "configurations:Release"
		optimize "On"
        vectorextensions "AVX2"
        isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }
	
		defines 
		{
			"HZ_RELEASE",
			"WL_RELEASE"
		}
	
	filter "configurations:Dist"
		kind "WindowedApp"
		optimize "On"
		symbols "Off"
		vectorextensions "AVX2"
		isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }
		defines { "HZ_DIST" }

		defines 
		{
			"HZ_DIST",
			"WL_DIST"
		}