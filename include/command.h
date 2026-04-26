#ifndef __COMMAND__
#define __COMMAND__

#include "cfg_logs_utils.h"
#include "cli_parser.h"
#include "file_utils.h"
#include "filter_utils.h"
#include "report_utils.h"
#include "types.h"

void execute_add(COMMAND *command);
void execute_list(COMMAND *command);
void execute_view(COMMAND *command);
void execute_remove_report(COMMAND *command);
void execute_update_threshold(COMMAND *command);
void execute_filter(COMMAND *command);
void execute(COMMAND *command);

#endif
