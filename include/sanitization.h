#ifndef __SANITIZATION__
#define __SANITIZATION__

#include <string.h>

int check_command_integrity(int argc, char **argv);
int check_arg_integrity(int argc, char **argv);

#endif
