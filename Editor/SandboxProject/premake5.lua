FileVersion = 1.2

HazelRootDirectory = os.getenv("PROJECT_DIR")
include (path.join(HazelRootDirectory, "Editor", "Resources", "LUA", "Hazel.lua"))

workspace "Sandbox"
	startproject "Sandbox"
	configurations { "Debug", "Release", "Dist" }

group "Hazel"
project "Engine-ScriptCore"
	location "%{HazelRootDirectory}/Engine-ScriptCore"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"

	targetdir ("%{HazelRootDirectory}/Editor/Resources/Scripts")
	objdir ("%{HazelRootDirectory}/Editor/Resources/Scripts/Intermediates")

	files {
		"%{HazelRootDirectory}/Engine-ScriptCore/Source/**.cs",
		"%{HazelRootDirectory}/Engine-ScriptCore/Properties/**.cs"
	}

	linkAppReferences()

	filter "configurations:Debug"
		optimize "Off"
		symbols "Default"

	filter "configurations:Release"
		optimize "On"
		symbols "Default"

	filter "configurations:Dist"
		optimize "Full"
		symbols "Off"

group ""

project "Sandbox"
	location "Assets/Scripts"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"

	targetname "Sandbox"
	targetdir ("%{prj.location}/Binaries")
	objdir ("%{prj.location}/Intermediates")

	files  {
		"Assets/Scripts/Source/**.cs", 
	}

	linkAppReferences()

	filter "configurations:Debug"
		optimize "Off"
		symbols "Default"

	filter "configurations:Release"
		optimize "On"
		symbols "Default"

	filter "configurations:Dist"
		optimize "Full"
		symbols "Off"
