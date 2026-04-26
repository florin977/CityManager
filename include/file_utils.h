#ifndef __FILE_UTILS__
#define __FILE_UTILS__

#include "types.h"

void create_file(COMMAND *command, char *file, mode_t mode);

int open_file(COMMAND *command, char *file, char *mode, int flags);

int check_file_permission(COMMAND *command, char *file, char *permissions);

int check_symlink(const char *filepath);

void create_symlink(const char *target, const char *linkpath);

#endif
