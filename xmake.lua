add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

add_requires("levilamina", {configs = {target_type = "server"}})
add_requires("levibuildscript")

add_requires("openssl3")
add_requires("cpp-httplib", {configs = {ssl = true}})

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

target("BDS-Chatbot")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_defines("NOMINMAX", "UNICODE")
    add_packages("levilamina", "openssl3", "cpp-httplib")
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")