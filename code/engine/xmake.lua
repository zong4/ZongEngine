-- includes sub-projects
includes("engine/core",
         "engine/platform",
         "engine/resource",
         "engine/function"
         )

target("engine")
    add_deps("core", "platform", "resource", "function")

    set_kind("static")

    add_headerfiles("./engine/engine.hpp")
    add_includedirs(".", {public = true}) 