@echo off

if not exist build mkdir build
pushd build

cl -nologo -FC -Fersc.exe -Oi -O2 ..\src\main.cpp -link -subsystem:console shell32.lib

del *.obj

popd
