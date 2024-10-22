include "./vendor/premake_customization/solution_items.lua"
include "Dependencies.lua"
include "./Hazelnut/Resources/LUA/Hazel.lua"

workspace "Engine"
	configurations { "Debug", "Debug-AS", "Release", "Dist" }
	targetdir "build"
	startproject "Hazelnut"
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
include "Engine/vendor/GLFW"
include "Engine/vendor/imgui"
include "Engine/vendor/Box2D"
include "Engine/vendor/tracy"
include "Engine/vendor/JoltPhysics/JoltPhysicsPremake.lua"
include "Engine/vendor/JoltPhysics/JoltViewerPremake.lua"
include "Engine/vendor/NFD-Extended"
group "Dependencies/msdf"
include "Engine/vendor/msdf-atlas-gen"
group ""

group "Core"
include "Engine"
include "Hazel-ScriptCore"
group ""

group "Tools"
include "Hazelnut"
group ""

group "Runtime"
include "Hazel-Runtime"
include "Hazel-Launcher"
group ""
