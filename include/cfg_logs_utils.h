#ifndef __CFG_LOGS_UTILS__
#define __CFG_LOGS_UTILS__

#include "file_utils.h"
#include "types.h"

void update_parameter(COMMAND *command, char *parameter, char *value);

void write_district_cfg(COMMAND *command, char *field, char *value);

void write_logged_district(COMMAND *command);
#endif
