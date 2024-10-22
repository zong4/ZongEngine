project "JoltPhysics"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "JoltPhysics/Jolt/**.cpp",
        "JoltPhysics/Jolt/**.h",
        "JoltPhysics/Jolt/**.inl",
        "JoltPhysics/Jolt/**.gliffy"
    }

    includedirs { "JoltPhysics/Jolt", "JoltPhysics/" }

    filter "system:windows"
        systemversion "latest"

        files { "JoltPhysics/Jolt/Jolt.natvis" }

    filter "configurations:Debug or configurations:Debug-AS"
        symbols "on"
        optimize "off"

        defines
        {
            "_DEBUG",
            "JPH_DEBUG_RENDERER",
            "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
            "JPH_EXTERNAL_PROFILE"
        }

    filter { "system:windows", "configurations:Debug-AS" }
		sanitize { "Address" }
		flags { "NoRuntimeChecks", "NoIncrementalLink" }

    filter "configurations:Release"
        optimize "speed"
        vectorextensions "AVX2"
        isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }

        defines
        {
            "JPH_DEBUG_RENDERER",
            "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
            "JPH_EXTERNAL_PROFILE"
        }

    filter "configurations:Dist"
        optimize "speed"
        symbols "off"
        vectorextensions "AVX2"
        isaextensions { "BMI", "POPCNT", "LZCNT", "F16C" }

--[[project "JoltPhysics-Samples"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    flags { "ExcludeFromBuild" }

    files
    {
        "JoltPhysics/Samples/**.cpp",
        "JoltPhysics/Samples/**.h",
        "JoltPhysics/Samples/**.inl",
        "JoltPhysics/Samples/**.gliffy"
    }

    includedirs { "JoltPhysics/Jolt", "JoltPhysics/Samples", "JoltPhysics/TestFramework", "JoltPhysics/" }

    defines
    {
        "JPH_DEBUG_RENDERER",
        "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
        "JPH_USE_LZCNT",
        "JPH_USE_TZCNT",
        "JPH_USE_FMADD"
    }

    filter "system:windows"
        systemversion "latest"
        
    filter "configurations:Debug"
        symbols "on"
        optimize "off"

    filter "configurations:Release"
        optimize "speed"

    filter "configurations:Dist"
        optimize "speed"
        symbols "off"    
]]--