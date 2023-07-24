target("function")
    add_deps("resource")
    set_kind("static")

    set_pcxxheader("function/pch.hpp")
    add_headerfiles("./**/*.hpp")
    add_files("./**/*.cpp")
    add_includedirs(".", {public = true}) 