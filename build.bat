@echo off

cls
if not exist .\out mkdir out 
pushd out 

where cl /q
if %ERRORLEVEL% neq 0 call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

set "build_options=-nologo -DEBUG -fp:fast -Od -Oi -Zi -FC -I"..\src" -std:c++17 -D_CRT_SECURE_NO_WARNINGS"

cl %build_options% ../src/main.cc -link -opt:ref gdi32.lib user32.lib kernel32.lib -out:lang.exe

popd 
