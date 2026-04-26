#include "../include/cli_parser.h"

void get_role(COMMAND *command, char *s) {
  if (!strcmp(s, "inspector")) {
    command->role = INSPECTOR;
  } else if (!strcmp(s, "manager")) {
    command->role = MANAGER;
  } else {
    fprintf(stderr, "Invalid user role! Valid roles are: inspector; manager\n");
    exit(-1);
  }
}

void get_permissions(COMMAND *command) {
  switch (command->role) {
  case MANAGER:
    command->permission.READ_BIT = S_IRUSR;
    command->permission.WRITE_BIT = S_IWUSR;
    command->permission.EXECUTE_BIT = S_IXUSR;
    break;

  case INSPECTOR:
    command->permission.READ_BIT = S_IRGRP;
    command->permission.WRITE_BIT = S_IWGRP;
    command->permission.EXECUTE_BIT = S_IXGRP;
    break;
  }
}

void get_username(COMMAND *command, char *s) {
  if (strlen(s) >= 30) {
    fprintf(stderr, "Invalid username length. Limit is 30\n");
    exit(-1);
  }

  strcpy(command->username, s);
}

void get_type(COMMAND *command, char *s) {
  if (!strcmp(s, "--add")) {
    command->type = ADD;
  } else if (!strcmp(s, "--list")) {
    command->type = LIST;
  } else if (!strcmp(s, "--view")) {
    command->type = VIEW;
  } else if (!strcmp(s, "--remove_report")) {
    command->type = REMOVE_REPORT;
  } else if (!strcmp(s, "--update_threshold")) {
    command->type = UPDATE_THRESHOLD;
  } else if (!strcmp(s, "--filter")) {
    command->type = FILTER;
  } else {
    fprintf(stderr,
            "Invalid command type! Supported commands are: "
            "add;list;view;remove_report;add_report;update_threshold;filter\n");
    exit(-1);
  }
}

void get_args(COMMAND *command, int argc, char **argv) {
  command->argc = argc - 7;
  if (argc <= 0) {
    fprintf(stderr, "Not enough arguments\n");
    exit(-1);
  }

  command->district = argv[6];
  command->argv = &argv[7];
}
