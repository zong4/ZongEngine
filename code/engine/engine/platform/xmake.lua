add_requires("vulkansdk", "glm", "glfw")

target("platform")
    add_deps("core")
    add_packages("vulkansdk", "glm", "glfw", {public = true}) 

    set_kind("static")

    set_pcxxheader("platform/pch.hpp")
    add_headerfiles("./**/*.hpp")
    add_files("./**/*.cpp")
    add_includedirs(".", {public = true}) 