-- minimum xmake version
set_xmakever("2.8.2")

add_repositories("libxse-xrepo https://github.com/libxse/libxse-xrepo")

-- dependencies
add_requires("spdlog")

set_project("BGSScriptExtenderPluginTools")
set_version("1.0.0")

set_languages("c++23")
set_warnings("allextra")
set_encodings("utf-8")

add_rules("mode.debug", "mode.release")

target("BGSScriptExtenderPluginTools")

    set_kind("static")

    add_files("source/**.cpp")
    add_headerfiles("include/**.h")
    add_includedirs("include")
    -- toml
    add_includedirs("../tomlplusplus/include")

    add_packages("spdlog")

    -- precompiled header
    set_pcxxheader("include/PCH.h")
    add_forceincludes("pch.h")

    -- flags

    add_cxxflags("/EHsc", "/permissive-", { public = true })
    
    add_cxxflags(
        "cl::/bigobj",
        "cl::/cgthreads8",
        "cl::/diagnostics:caret",
        "cl::/external:W0",
        "cl::/fp:contract",
        "cl::/fp:except-",
        "cl::/guard:cf-",
        "cl::/Zc:preprocessor",
        "cl::/Zc:templateScope"
    )

    -- add flags (cl: disable warnings)
    add_cxxflags(
        "cl::/wd4200", -- nonstandard extension used : zero-sized array in struct/union
        "cl::/wd4201", -- nonstandard extension used : nameless struct/union
        "cl::/wd4324", -- structure was padded due to alignment specifier
        { public = true }
    )

    -- add flags (cl: warnings -> errors)
    add_cxxflags(
        "cl::/we4715", -- not all control paths return a value
        { public = true }
    )
