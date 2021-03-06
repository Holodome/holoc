#!/bin/bash

mkdir -p out
pushd ./out

build_options="-Wall -pedantic -lm -O0 -I../src -fno-exceptions"
clang++ -g $build_options -o lang ../src/main.cc

popd
