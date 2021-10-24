@ECHO OFF
CLS
REM Seprate complation units build
SETLOCAL EnableDelayedExpansion

PUSHD src
SET build_files=
FOR /R %%f IN (*.c) DO (
    SET build_files=!build_files! %%f	
)
POPD
ECHO Files: %build_files%


SET "error_policy=-Wall -Werror -Wno-unused-function -Wno-missing-braces"
SET "linked_libs=-L./ -luser32 -lkernel32 -lgdi32"
clang %build_files% -g -gcodeview -D_CRT_SECURE_NO_WARNINGS -O0 %error_policy% -Isrc -I../thirdparty %linked_libs% -o build\lang.exe
