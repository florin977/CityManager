#ifndef __REPORT_UTILS__
#define __REPORT_UTILS__

#include "file_utils.h"
#include "types.h"

void print_reports_file_info(COMMAND *command);

int get_report_id(COMMAND *command);

void write_report(COMMAND *command);

int get_report_by_offset(COMMAND *command, __off_t offset, REPORT_DATA *data);

__off_t get_report_by_id(COMMAND *command, char *report_id, REPORT_DATA *data);

void print_report(REPORT_DATA data);

void get_report_data(COMMAND *command);

void delete_report_from_offset(COMMAND *command, off_t offset);

#endif
