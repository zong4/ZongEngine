add_requires("assimp")

target("resource")
    add_deps("platform")
    add_packages("assimp", {public = true})

    set_kind("static")

    set_pcxxheader("resource/pch.hpp")
    add_headerfiles("./**/*.hpp")
    add_files("./**/*.cpp")
    add_includedirs(".", {public = true}) 