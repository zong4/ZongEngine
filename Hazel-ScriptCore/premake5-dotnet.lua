-- Non-Windows split premake project

HazelRootDirectory = os.getenv("HAZEL_DIR")
include (path.join(HazelRootDirectory, "Hazel", "vendor", "Coral", "Premake", "CSExtensions.lua"))

workspace "Hazel-ScriptCore"
	configurations { "Debug", "Release" }

	filter "configurations:Debug or configurations:Debug-AS"
		optimize "Off"
		symbols "On"

	filter "configurations:Release"
		optimize "On"
		symbols "Default"

	filter "configurations:Dist"
		optimize "Full"
		symbols "Off"

	include "../Hazel/vendor/Coral/Coral.Managed"

	project "Hazel-ScriptCore"
		kind "SharedLib"
		language "C#"
		dotnetframework "net8.0"
		clr "Unsafe"
		targetdir ("../Hazelnut/Resources/Scripts")
		objdir ("../Hazelnut/Resources/Scripts/Intermediates")

		--linkAppReferences(false)

		links { "Coral.Managed" }

		propertytags {
			{ "AppendTargetFrameworkToOutputPath", "false" },
			{ "Nullable", "enable" },
		}

		files {
			"Source/**.cs",
			"Properties/**.cs"
		}
