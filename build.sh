#!/bin/sh
mkdir -p build
pushd build
# get list of all .c files
filenames=$(find ../src/ -type f -name "*.c")
echo Filenames: $filenames
build_options="-O0 -x c -std=c99 -I../src -D_CRT_SECURE_NO_WARNINGS"
error_policy="-Wall -Werror -Wno-unused-function -Wno-missing-braces"
clang -g $build_options $error_policy -o lang $filenames
popd
