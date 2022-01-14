#ifndef FILEPATH_H
#define FILEPATH_H

#include "types.h"

struct allocator;

string get_current_dir(struct allocator *a);
string get_realpath(string path, struct allocator *a);
bool path_is_dir(string path);
string path_dirname(string path);
string path_filename(string path);
string path_basename(string path);
string path_to_absolute(string path, struct allocator *a);
string path_clean(string path, struct allocator *a);

#endif
