@echo off

if not exist build mkdir build
pushd build

set CompilerFlags= -Od -Ob0 -Oi -Zi -FC -Fe:"rsc" -nologo -W3 -fp:fast -fp:except- -Gm- -GR- -EHa- -GS- -Gs9999999
set Defines= -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32 -DCOMPILER_MSVC=1 -D_CRT_SECURE_NO_WARNINGS=1
set LinkerFlags= -subsystem:console -incremental:no -opt:ref -STACK:0x100000,0x100000
set Libs= shell32.lib

cl %CompilerFlags% %Defines% ..\src\main.cpp -link %LinkerFlags% %Libs%

del *.obj

popd
