version = 1;

configurations = {
    "Debug",
    "Release"
};

project "Test" {
    kind = ConsoleApp;
    outputdir = "test/build";
    objdir = "test/build-int";
    outputname = "%{ProjectName}";

    staticRuntime = true;

    defines = {
        UNICODE,
        _UNICODE,
        RENDER_D3D11,
    };

    includeDirs = {
        "external/include",
        "src",
    };

    libDirs = {
        "external/lib",
    };

    libs = {
        "user32.lib", "gdi32.lib", "opengl32.lib"
    };

    files = {
        "test/test.cpp",
        "test/bruh.cpp"
    };

    if configuration is Debug {
        debugSymbols = true;
        optimize = false;
        runtime = Debug;

        defines = {
            _DEBUG,
            DEBUG,
        };
    }

    if configuration is Release {
        debugSymbols = true;
        optimize = true;
        runtime = Release;

        defines = {
            NDEBUG,
            RELEASE,
        };
    }
}
