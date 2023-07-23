-- vulkan
add_requires("vulkan-hpp", "vulkansdk", "vulkan-loader")
add_requires("vulkan-validationlayers") 

-- vulkan and opengl
add_requires("glm", "glfw")

target("platform")
    add_deps("core")
    add_packages("vulkan-hpp", "vulkansdk", "vulkan-loader", {public = true}) 
    add_packages("vulkan-validationlayers", {public = true})
    add_packages("glm", "glfw", {public = true})

    set_kind("static")

    set_pcxxheader("platform/pch.hpp")
    add_headerfiles("./**/*.hpp")
    add_files("./**/*.cpp")
    add_includedirs(".", {public = true}) 