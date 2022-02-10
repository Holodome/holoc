#ifndef FILEPATH_H
#define FILEPATH_H

#include "general.h"

// Returns path of current directory
void get_current_dir(char *buf, uint32_t buf_size);
// Check if given path corresponds to directory
bool path_is_dir(string path);
string path_dirname(string path);
string path_filename(string path);
string path_basename(string path);
string path_to_absolute(string path);
string path_clean(string path);

#endif
