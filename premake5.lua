workspace "Concordia"
    architecture "x86_64"
    configurations { "Debug", "Release",  }
    startproject "ConcordiaClient"
    location(_ACTION)

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        symbols "On"
        optimize "On"

    filter "system:windows"
        systemversion "latest"

project "ConcordiaClient"
    location(_ACTION)
    language "C++"
    cppdialect "C++20"
    kind "ConsoleApp"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs {
        "vendor/miniaudio",
        "vendor/asio/include",
        "src/common", 
    }
    
    files { 
        "src/common/**.cpp", 
        "src/common/**.h", 
        "src/client/**.cpp", 
        "src/client/**.h"  
    }
    
    filter "system:windows"
        defines { "_WIN32_WINDOWS" }
        links { "ws2_32" }


project "ConcordiaServer"
    location(_ACTION)
    language "C++"
    cppdialect "C++20"
    kind "ConsoleApp"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs {
        "vendor/asio/include",
        "src/common", 
    }

    files { 
        "src/common/**.cpp", 
        "src/common/**.h", 
        "src/server/**.cpp", 
        "src/server/**.h",  
    }

    filter "system:windows"
        defines { "_WIN32_WINDOWS" }
        links { "ws2_32" }