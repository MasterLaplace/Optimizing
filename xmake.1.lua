add_requires("glm", "glfw", "glew")

set_languages("c++20")

-- target("opti")
--     set_kind("binary")
--     add_files("ok.cpp")
--     add_packages("glm", "glfw", "glew")

target("heightmap")
    set_kind("binary")
    add_files("heightmap_optimized.cpp")
    add_packages("glm", "glfw", "glew")
    add_links("GL", "GLU")  -- Liens OpenGL pour Linux