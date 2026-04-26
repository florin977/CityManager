#ifndef __CLI_PARSER__
#define __CLI_PARSER__

#include "types.h"

void get_role(COMMAND *command, char *s);
void get_permissions(COMMAND *command);
void get_username(COMMAND *command, char *s);
void get_type(COMMAND *command, char *s);
void get_args(COMMAND *command, int argc, char **argv);
#endif
