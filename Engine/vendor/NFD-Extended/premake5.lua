project "NFD-Extended"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files { "NFD-Extended/src/include/nfd.h", "NFD-Extended/src/include/nfd.hpp" }
    includedirs { "NFD-Extended/src/include/" }

    filter "system:windows"
		systemversion "latest"

        files { "NFD-Extended/src/nfd_win.cpp" }

    filter "system:linux"
		pic "On"
		systemversion "latest"

        files { "NFD-Extended/src/nfd_gtk.cpp" }

        result, err = os.outputof("pkg-config --cflags gtk+-3.0")
        buildoptions { result }

    filter "system:macosx"
		pic "On"

        files { "NFD-Extended/src/nfd_cocoa.m" }

    filter "configurations:Debug or configurations:Debug-AS"
        runtime "Debug"
        symbols "On"

    filter { "system:windows", "configurations:Debug-AS" }	
		sanitize { "Address" }
		flags { "NoRuntimeChecks", "NoIncrementalLink" }

    filter "configurations:Release"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        runtime "Release"
        optimize "Full"