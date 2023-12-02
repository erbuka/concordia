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

        project "Glad"
        location(_ACTION)
        language "C"
        kind "StaticLib"
    
        objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
        targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
        debugdir "bin/%{cfg.buildcfg}/%{prj.name}"
    
        includedirs { "vendor/glad/include" }
        files { "vendor/glad/src/glad.c" }
    
    
project "GLFW"
    location(_ACTION)
    language "C"
    kind "StaticLib"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    files {
        "vendor/glfw/src/internal.h", 
        "vendor/glfw/src/platform.h", 
        "vendor/glfw/src/mappings.h",
        "vendor/glfw/src/context.c",
        "vendor/glfw/src/init.c",
        "vendor/glfw/src/input.c", 
        "vendor/glfw/src/monitor.c", 
        "vendor/glfw/src/platform.c",
        "vendor/glfw/src/vulkan.c",
        "vendor/glfw/src/window.c",
        "vendor/glfw/src/egl_context.c",
        "vendor/glfw/src/osmesa_context.c",
        "vendor/glfw/src/null_platform.h",
        "vendor/glfw/src/null_joystick.h",
        "vendor/glfw/src/null_init.c",
        "vendor/glfw/src/null_monitor.c",
        "vendor/glfw/src/null_window.c",
        "vendor/glfw/src/null_joystick.c",
    }

    filter "system:windows"

        defines "_GLFW_WIN32"

        files {
            "vendor/glfw/src/win32_time.h",
            "vendor/glfw/src/win32_thread.h",
            "vendor/glfw/src/win32_module.c",
            "vendor/glfw/src/win32_time.c",
            "vendor/glfw/src/win32_thread.c",
            "vendor/glfw/src/win32_platform.h",
            "vendor/glfw/src/win32_joystick.h",
            "vendor/glfw/src/win32_init.c",
            "vendor/glfw/src/win32_joystick.c",
            "vendor/glfw/src/win32_monitor.c",
            "vendor/glfw/src/win32_window.c",
            "vendor/glfw/src/wgl_context.c",
        }
    
project "MediaLib"
    location(_ACTION)
    language "C++"
    cppdialect "C++20"
        kind "StaticLib"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs {
        "src/ml",
        "vendor/glfw/include",
        "vendor/glad/include",
        "vendor/miniaudio",
        "vendor/stb",
    }

    files { "src/ml/**.cpp", "src/ml/**.h"  }
        links { "GLFW", "Glad"  }

        filter "system:windows"
            includedirs { "vendor/openssl-windows/x64/include" }
            links { "opengl32" }
    

        
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