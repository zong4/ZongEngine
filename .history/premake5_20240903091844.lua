include "./vendor/premake_customization/solution_items.lua"
include "Dependencies.lua"
include "./Editor/Resources/LUA/Hazel.lua"

workspace "Hazel"
	configurations { "Debug", "Debug-AS", "Release", "Dist" }
	targetdir "build"
	startproject "Editor"
    conformancemode "On"

	language "C++"
	cppdialect "C++20"
	staticruntime "Off"

	solution_items { ".editorconfig" }

	configurations { "Debug", "Release", "Dist" }

	flags { "MultiProcessorCompile" }

	-- NOTE(Peter): Don't remove this. Please never use Annex K functions ("secure", e.g _s) functions.
	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING",
		"TRACY_ENABLE",
		"TRACY_ON_DEMAND",
		"TRACY_CALLSTACK=10",
	}

    filter "action:vs*"
        linkoptions { "/ignore:4099" } -- NOTE(Peter): Disable no PDB found warning
        disablewarnings { "4068" } -- Disable "Unknown #pragma mark warning"

	filter "language:C++ or language:C"
		architecture "x86_64"

	filter "configurations:Debug or configurations:Debug-AS"
		optimize "Off"
		symbols "On"

	filter { "system:windows", "configurations:Debug-AS" }	
		sanitize { "Address" }
		flags { "NoRuntimeChecks", "NoIncrementalLink" }

	filter "configurations:Release"
		optimize "On"
		symbols "Default"

	filter "configurations:Dist"
		optimize "Full"
		symbols "Off"

	filter "system:windows"
		buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
include "Hazel/vendor/GLFW"
include "Hazel/vendor/imgui"
include "Hazel/vendor/Box2D"
include "Hazel/vendor/tracy"
include "Hazel/vendor/JoltPhysics/JoltPhysicsPremake.lua"
include "Hazel/vendor/JoltPhysics/JoltViewerPremake.lua"
include "Hazel/vendor/NFD-Extended"
group "Dependencies/msdf"
include "Hazel/vendor/msdf-atlas-gen"
group ""

group "Core"
include "Hazel"
include "Hazel-ScriptCore"
group ""

group "Tools"
include "Editor"
group ""

group "Runtime"
include "Hazel-Runtime"
include "Hazel-Launcher"
group ""
