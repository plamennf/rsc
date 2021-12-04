@echo off

bin\rsc.exe tests\test.rsc -configuration:debug
bin\rsc.exe tests\test.rsc -configuration:release
bin\debug\main_debug.exe
bin\main.exe
