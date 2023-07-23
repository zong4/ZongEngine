-- includes sub-projects
includes("engine/core",
         "engine/platform"
         )

target("engine")
    add_deps("core", "platform")

    set_kind("static")

    add_headerfiles("./engine/engine.hpp")
    add_includedirs(".", {public = true}) 