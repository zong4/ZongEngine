-- define project
set_project("ZongEngine")
set_xmakever("2.7.0")
set_version("1.0.0", {build = "%Y%m%d%H%M"})

-- set common flags
-- set_warnings("all", "error")
set_languages("c++23")
add_mxflags("-Wno-error=deprecated-declarations", "-fno-strict-aliasing", "-Wno-error=expansion-to-defined")

-- add build modes
add_rules("mode.release", "mode.debug")

-- set_runtimes("MT")

-- includes sub-projects
includes("code/engine")
includes("code/editor")

-- add macro
if is_mode("debug") then
    add_defines("DEBUG")
elseif is_mode("release") then
    add_defines("RELEASE")
end


if is_plat("windows") then
    add_defines("WINDOWS")
    -- add_ldflags("-subsystem:windows")
elseif is_plat("linux") then
    add_defines("LINUX")
end


