@echo off

build\rsc.exe tests\test.rsc -configuration:debug
build\rsc.exe tests\test.rsc -configuration:release
build\debug\main_debug.exe
build\main.exe
