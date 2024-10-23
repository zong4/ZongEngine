FileVersion = 1.2

EngineRootDirectory = os.getenv("PROJECT_DIR")
include (path.join(EngineRootDirectory, "Editor", "Resources", "LUA", "Engine.lua"))

workspace "Sandbox"
	startproject "Sandbox"
	configurations { "Debug", "Release", "Dist" }

group "Engine"
project "Engine-ScriptCore"
	location "%{EngineRootDirectory}/Engine-ScriptCore"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"

	targetdir ("%{EngineRootDirectory}/Editor/Resources/Scripts")
	objdir ("%{EngineRootDirectory}/Editor/Resources/Scripts/Intermediates")

	files {
		"%{EngineRootDirectory}/Engine-ScriptCore/Source/**.cs",
		"%{EngineRootDirectory}/Engine-ScriptCore/Properties/**.cs"
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
