add_requires("spdlog", {debug = true})

target("core")
    add_packages("spdlog", {public = true}) 

    set_kind("static")

    set_pcxxheader("core/pch.hpp")
    add_headerfiles("./**/*.hpp")
    add_files("./**/*.cpp")
    add_includedirs(".", {public = true}) 

