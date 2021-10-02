#!/bin/sh
mkdir -p build
# get list of all .c files
filenames=$(find src/ -type f -name "*.c")
echo Filenames: $filenames
build_options="-O0 -x c -std=c99 -Isrc -D_CRT_SECURE_NO_WARNINGS"
error_policy="-Wshadow -Wextra -Wall -Werror -Wno-unused-function -Wno-missing-braces -Wformat=2"
clang -g $build_options $error_policy -o build/lang $filenames
#cp test.txt build/
