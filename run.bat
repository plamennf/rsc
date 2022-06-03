@echo off

build\rsc.exe tests\test.rsc -configuration:Debug -B
bin\debug\main_debug.exe
