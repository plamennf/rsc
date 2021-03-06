outputdir bin
objdir bin-int
exename main
	
configurations {
    debug: {
        outputdir bin\debug
	objdir bin-int\debug
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

headers {
    tests\bruh.h
}

files {
    tests\test.cpp
    tests\bruh.cpp
}
