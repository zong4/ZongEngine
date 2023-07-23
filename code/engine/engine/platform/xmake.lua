-- vulkan
add_requires("vulkan-validationlayers", {debug = true})
add_requires("vulkansdk", "vulkan-loader")

-- vulkan and opengl
add_requires("glm", "glfw")

target("platform")
    add_deps("core")
    add_packages("vulkansdk", "vulkan-loader", "vulkan-validationlayers", "glm", "glfw", {public = true}) 

    set_kind("static")

    set_pcxxheader("platform/pch.hpp")
    add_headerfiles("./**/*.hpp")
    add_files("./**/*.cpp")
    add_includedirs(".", {public = true}) 