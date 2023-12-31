workspace "Concordia"
    architecture "x86_64"
    configurations { "Debug", "Release", "Dist" }
    startproject "ConcordiaClient"
    location(_ACTION)

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "DEBUG" }
        symbols "On"
        optimize "On"
    
    filter "configurations:Dist"
        symbols "Off"
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
    


project "ConcordiaTest"
    location(_ACTION)
    language "C++"
    cppdialect "C++20"
    kind "ConsoleApp"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs {
        "src/common", 
    }
    
    files { 
        "src/test/**.cpp", 
        "src/test/**.h", 
    }

project "ConcordiaClient"
    location(_ACTION)
    language "C++"
    cppdialect "C++20"
    kind "ConsoleApp"

    objdir "bin-int/%{cfg.buildcfg}/%{prj.name}"
    targetdir "bin/%{cfg.buildcfg}/%{prj.name}"
    debugdir "bin/%{cfg.buildcfg}/%{prj.name}"

    includedirs {
        "vendor/glad/include",
        "vendor/miniaudio",
        "vendor/asio/include",
        "src/common", 
        "src/ml",
    }
    
    files { 
        "src/common/**.cpp", 
        "src/common/**.h", 
        "src/client/**.cpp", 
        "src/client/**.h"  
    }

    prebuildcommands {
        "python ../pack_assets.py ../assets > ../src/client/assets.generated.h"
    }

    links { "MediaLib", "Glad", "GLFW" }
    
    filter "system:windows"
        defines { "_WIN32_WINDOWS" }
        links { "ws2_32", "opengl32", "gdi32" }

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