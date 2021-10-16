/*
Author: Holodome
Date: 15.10.2021
File: src/lib/path.h
Version: 0

Utilities for manipulating path strings
*/
#ifndef PATH_H
#define PATH_H

#include "lib/general.h"
#include "lib/strings.h"

Str path_extension(Str path);
Str path_strip_extension(Str path);
Str path_parent(Str path);
Str path_dir(Str path);

const char *path_extensionz(const char *path);
const char *path_basez(const char *path);

#endif