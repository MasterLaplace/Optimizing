add_rules("mode.debug", "mode.release", "plugin.vsxmake.autoupdate")

add_requires("glm", "sfml")

set_project("Optimizing")
set_license("MIT")

target("optimizing")
    set_kind("binary")
    set_default(true)
    set_languages("cxx20")
    set_policy("build.warning", true)
    set_version("0.0.0")

    add_packages("glm", "sfml")

    if is_mode("debug") then
        add_defines("DEBUG")
        set_symbols("debug")
        set_optimize("none")
    elseif is_mode("release") then
        add_defines("NDEBUG")
        set_optimize("fastest")
    end

    add_files("./*.cpp")

    add_headerfiles("./*.hpp", { public = true })
    add_includedirs("./", { public = true })
