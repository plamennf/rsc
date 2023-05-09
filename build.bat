@echo off

if not exist build mkdir build
pushd build

set CompilerFlags= /Od /Ob0 /MTd /Zi /FC /nologo /Fe:"rsc" /W3
set Defines= /DBUILD_DEBUG /DOS_WINDOWS /DCOMPILER_MSVC /D_CRT_SECURE_NO_WARNINGS /DUNICODE /D_UNICODE
set LinkerFlags= /opt:ref /incremental:no /subsystem:console
set Libs=

cl %CompilerFlags% %Defines% ..\src\*.cpp /link %LinkerFlags% %Libs%

del *.obj

popd
