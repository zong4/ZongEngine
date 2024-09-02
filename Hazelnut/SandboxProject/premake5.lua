FileVersion = 1.2

HazelRootDirectory = os.getenv("HAZEL_DIR")
include (path.join(HazelRootDirectory, "Hazel", "vendor", "Coral", "Premake", "CSExtensions.lua"))

workspace "Sandbox"
	startproject "Sandbox"
	configurations { "Debug", "Release", "Dist" }

group "Hazel"

include (path.join(HazelRootDirectory, "Hazel", "vendor", "Coral", "Coral.Managed"))
include (path.join(HazelRootDirectory, "Hazel-ScriptCore"))

group ""

project "Sandbox"
	location "Assets/Scripts"
	kind "SharedLib"
	language "C#"
	dotnetframework "net8.0"

	targetname "Sandbox"
	targetdir ("%{prj.location}/Binaries")
	objdir ("%{prj.location}/Intermediates")

	propertytags {
        { "AppendTargetFrameworkToOutputPath", "false" },
        { "Nullable", "enable" },
    }

	files  {
		"Assets/Scripts/Source/**.cs", 
	}

	links {
		"%{HazelRootDirectory}/Hazelnut/Resources/Scripts/Hazel-ScriptCore.dll",
		"%{HazelRootDirectory}/Hazelnut/DotNet/Coral.Managed.dll",
	}	

	filter "configurations:Debug"
		optimize "Off"
		symbols "Default"

	filter "configurations:Release"
		optimize "On"
		symbols "Default"

	filter "configurations:Dist"
		optimize "Full"
		symbols "Off"
