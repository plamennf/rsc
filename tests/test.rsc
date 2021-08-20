outputdir build
objdir obj
exename main
	
configurations {
    debug: {
        outputdir build\debug
	objdir obj\debug
        exename main_debug
    },
    release: {
        
    },
}

includedirs {
    external\include
}

libdirs {
    external\lib
}

libs {
    opengl32.lib
}

files {
   tests\test.cpp
   tests\bruh.cpp
}
