target("editor")
    add_deps("engine")

    set_kind("binary")
    add_headerfiles("*.hpp")
    add_files("*.cpp")
    