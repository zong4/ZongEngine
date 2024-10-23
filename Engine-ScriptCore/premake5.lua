project "Engine-ScriptCore"

-- NOTE(Yan): Windows-only until we switch to Coral
filter "not system:windows"
    kind "StaticLib"
filter "system:windows"
	kind "SharedLib"
	language "C#"
	dotnetframework "4.7.2"

	linkAppReferences(false)

	targetdir ("../Hazelnut/Resources/Scripts")
	objdir ("../Hazelnut/Resources/Scripts/Intermediates")

	files 
	{
		"Source/**.cs",
		"Properties/**.cs"
	}