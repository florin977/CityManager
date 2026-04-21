#ifndef __COMMAND__
#define __COMMAND__

#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum ROLE {
  MANAGER,
  INSPECTOR,
} ROLE;

typedef enum COMMAND_TYPE {
  ADD,
  LIST,
  VIEW,
  REMOVE_REPORT,
  UPDATE_THRESHOLD,
  FILTER,
} COMMAND_TYPE;

typedef struct GPS_COORDS {
  float lat;
  float lng;
} GPS_COORDS;

typedef struct REPORT_DATA {
  int report_id;
  char username[30];
  GPS_COORDS coords;
  char issue_category[30];
  int severity_level;
  time_t timestamp;
  char description[200];
} REPORT_DATA;

typedef union COMMAND_ARGS {
  int threshold_value;
  char district_id[30];
  char report_id[30];
  char filter_condition[200];
  REPORT_DATA report_data;
} COMMAND_ARGS;

typedef struct ROLE_BITS {
  mode_t READ_BIT;
  mode_t WRITE_BIT;
  mode_t EXECUTE_BIT;
} ROLE_BITS;

typedef struct COMMAND {
  ROLE role;
  ROLE_BITS permission;
  COMMAND_TYPE type;
  COMMAND_ARGS args;
  char username[30];
} COMMAND;

void get_role(COMMAND *command, char *s);
void get_permissions(COMMAND *command);
void get_username(COMMAND *command, char *s);
void get_type(COMMAND *command, char *s);
int get_report_id(COMMAND *command, char *district);
void get_report_data(COMMAND *command, char *district);
char *format_permission_bits(COMMAND *command, char *district);

void write_report(COMMAND *command, char *district);
void execute_add(COMMAND *command, char **argv);
void execute_list(COMMAND *command, char **argv);
void execute_view(COMMAND *command, char **argv);
void execute_remove_report(COMMAND *command, char **argv);
void execute_update_threshold(COMMAND *command, char **argv);
void execute_filter(COMMAND *command, char **argv);
void execute(COMMAND *command, int argc, char **argv);

#endif
