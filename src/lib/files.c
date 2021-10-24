#include "lib/files.h"

#include <stdio.h>

#include "lib/strings.h"
#include "lib/memory.h"
#include "lib/hashing.h"

#if OS_WINDOWS
#include "files_win32.inl"
#elif OS_POSIX 
#include "files_posix.inl"
#else 
#error !
#endif 