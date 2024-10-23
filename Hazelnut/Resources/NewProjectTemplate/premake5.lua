HazelRootDirectory = os.getenv("HAZEL_DIR")
include (path.join(HazelRootDirectory, "Hazelnut", "Resources", "LUA", "Hazel.lua"))

workspace "$PROJECT_NAME$"
	targetdir "build"
	startproject "$PROJECT_NAME$"
	
	configurations 
	{ 
		"Debug", 
		"Release",
		"Dist"
	}

group "Hazel"
project "Engine-ScriptCore"
	location "%{HazelRootDirectory}/Engine-ScriptCore"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"

	targetdir ("%{HazelRootDirectory}/Hazelnut/Resources/Scripts")
	objdir ("%{HazelRootDirectory}/Hazelnut/Resources/Scripts/Intermediates")

	files
	{
		"%{HazelRootDirectory}/Engine-ScriptCore/Source/**.cs",
		"%{HazelRootDirectory}/Engine-ScriptCore/Properties/**.cs"
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
