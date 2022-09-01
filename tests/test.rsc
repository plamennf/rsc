[1] # Version number

configurations
   Debug
   Release

project Test
    type "ConsoleApp"
    outputdir "run_tree"
    objdir "run_tree/obj/%{ProjectName}"
    outputname "%{ProjectName}"

    staticruntime "on"

    cfiles {
        src/test.cpp
        src/bruh.cpp
    }

    includedirs {
        external/include
    }

    libdirs {
        external/lib
    }

    libs {
        opengl32.lib
    }

    filter "configuration:Debug"
        debugsymbols "on"
        optimize "off"
        runtime "Debug"

    filter "configuration:Release"
        debugsymbols "off"
        optimize "on"
        runtime "Release"
        