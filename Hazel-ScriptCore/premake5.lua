HazelRootDirectory = os.getenv("HAZEL_DIR")

include (path.join(HazelRootDirectory, "Hazel", "vendor", "Coral", "Premake", "CSExtensions.lua"))
include (path.join(HazelRootDirectory, "Hazel", "vendor", "Coral", "Coral.Managed"))

project "Hazel-ScriptCore"
	kind "SharedLib"
	language "C#"
	dotnetframework "net8.0"
	clr "Unsafe"
	targetdir "%{HazelRootDirectory}/Hazelnut/Resources/Scripts"
	objdir "%{HazelRootDirectory}/Hazelnut/Resources/Scripts/Intermediates"

	links {
		"Coral.Managed"
	}

	propertytags {
		{ "AppendTargetFrameworkToOutputPath", "false" },
		{ "Nullable", "enable" },
	}

	files {
		"%{HazelRootDirectory}/Hazel-ScriptCore/Source/**.cs",
		"%{HazelRootDirectory}/Hazel-ScriptCore/Properties/**.cs"
	}