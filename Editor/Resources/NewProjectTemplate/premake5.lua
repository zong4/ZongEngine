EngineRootDirectory = os.getenv("ENGINE_DIR")
include (path.join(EngineRootDirectory, "Editor", "Resources", "LUA", "Engine.lua"))

workspace "$PROJECT_NAME$"
	targetdir "build"
	startproject "$PROJECT_NAME$"
	
	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

group "Engine"
project "Engine-ScriptCore"
	location "%{EngineRootDirectory}/Engine-ScriptCore"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"

	targetdir ("%{EngineRootDirectory}/Editor/Resources/Scripts")
	objdir ("%{EngineRootDirectory}/Editor/Resources/Scripts/Intermediates")

	files
	{
		"%{EngineRootDirectory}/Engine-ScriptCore/Source/**.cs",
		"%{EngineRootDirectory}/Engine-ScriptCore/Properties/**.cs"
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

group ""

project "$PROJECT_NAME$"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"

	targetname "$PROJECT_NAME$"
	targetdir ("%{prj.location}/Assets/Scripts/Binaries")
	objdir ("%{prj.location}/Intermediates")

	files 
	{
		"Assets/**.cs", 
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
