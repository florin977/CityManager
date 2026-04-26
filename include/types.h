#ifndef __TYPES__
#define __TYPES__

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define MAX_FIELD_LENGTH 64
#define MAX_OP_LENGTH 8
#define MAX_VAL_LENGTH 128

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

typedef struct ROLE_BITS {
  mode_t READ_BIT;
  mode_t WRITE_BIT;
  mode_t EXECUTE_BIT;
} ROLE_BITS;

typedef struct COMMAND {
  ROLE role;
  ROLE_BITS permission;
  COMMAND_TYPE type;
  REPORT_DATA report_data;
  char username[30];
  char *district;
  int argc;
  char **argv;
} COMMAND;

#endif
