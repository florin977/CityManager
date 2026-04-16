#include "../include/command.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

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
  int valid_length = 0;
  // Check that the string is at most 30 characters long
  for (int i = 0; i < 30; i++) {
    if (s[i] == 0) {
      valid_length = 1;
      break;
    }
  }

  if (!valid_length) {
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
  } else if (!strcmp(s, "--update_treshold")) {
    command->type = UPDATE_TRESHOLD;
  } else if (!strcmp(s, "--filter")) {
    command->type = FILTER;
  } else {
    fprintf(stderr,
            "Invalid command type! Supported commands are: "
            "add;list;view;remove_report;add_report;update_treshold;filter\n");
    exit(-1);
  }
}

void create_file(const char *pathname, mode_t mode) {
  if (open(pathname, O_CREAT | O_EXCL | O_RDWR, mode) == -1) {
    // File already exists, just make sure to have the correct permissions and
    // continue
    if (errno == EEXIST) {
      chmod(pathname, mode);

    } else {
      // Some other error, just exit and print the error
      perror("Failed to create file reports.dat");
    }
  }

  // Fix a bug with umask preventing the correct permissions
  chmod(pathname, mode);
}

int check_file_permission(COMMAND *command, char *arg) {
  switch (command->type) {
  case ADD:
    // Check that we can write to the reports.dat file;
    char path[256];
    sprintf(path, "%s/reports.dat", arg);
    struct stat sb;
    stat(path, &sb);
    return (sb.st_mode & command->permission.WRITE_BIT);
    break;

  case LIST:
    break;

  case VIEW:
    break;

  case REMOVE_REPORT:
    break;

  case UPDATE_TRESHOLD:
    break;

  case FILTER:
    break;
  }

  return 0;
}

void get_report_data(COMMAND *command) {
  printf("Please enter the report data:\nX: ");
  if (scanf("%f", &command->args.report_data.coords.lat) != 1) {
    fprintf(stderr, "Invalid latitude\n");
    exit(-1);
  }

  printf("Y: ");
  if (scanf("%f", &command->args.report_data.coords.lng) != 1) {
    fprintf(stderr, "Invalid longitutde\n");
    exit(-1);
  }

  printf("Category (road/lighting/flooding/other): ");
  if (scanf("%29s", command->args.report_data.issue_category) != 1) {
    fprintf(stderr, "Invalid category\n");
    exit(-1);
  }

  printf("Severity lvel(1/2/3): ");
  if (scanf("%d", &command->args.report_data.severity_level) != 1) {
    fprintf(stderr, "Invalid severity level\n");
    exit(-1);
  }
  // Consume newline
  fgetc(stdin);

  printf("Descriprtion: ");
  if (fgets(command->args.report_data.description, 200, stdin) == NULL) {
    fprintf(stderr, "Invalid description\n");
    exit(-1);
  }

  // get rid of the newline at the end
  command->args.report_data
      .description[strlen(command->args.report_data.description) - 1] = 0;

  strcpy(command->args.report_data.username, command->username);
}

void write_report(COMMAND *command, char *district) {
  // Open the reports.dat file (-1 on error)
  int reports_dat = -1;
  char path[256];
  sprintf(path, "%s/reports.dat", district);
  if ((reports_dat = open(path, O_APPEND | O_WRONLY)) == -1) {
    perror(path);
    exit(-1);
  }

  // Write the data
  // Report id is a placeholder until I figure out a way to randomise it
  // (and keep it unique)
  if ((write(reports_dat, &command->args.report_data.report_id, sizeof(int))) ==
      -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  // username; 1 char == 1 byte
  // sizeof() returns exactly 30 cahracters, null terminator included. Could
  // also work by doing strlen() + 1
  if ((write(reports_dat, command->args.report_data.username,
             sizeof(command->args.report_data.username))) == -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  // GPS latitude
  if ((write(reports_dat, &command->args.report_data.coords.lat,
             sizeof(float))) == -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  // GPS longitutde
  if ((write(reports_dat, &command->args.report_data.coords.lng,
             sizeof(float))) == -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  // issue_category; 1 char == 1 byte
  if ((write(reports_dat, command->args.report_data.issue_category,
             sizeof(command->args.report_data.issue_category))) == -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  // severity_level
  if ((write(reports_dat, &command->args.report_data.severity_level,
             sizeof(int))) == -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  // timestamp
  if ((write(reports_dat, &command->args.report_data.timestamp,
             sizeof(time_t))) == -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  // description
  if ((write(reports_dat, command->args.report_data.description,
             sizeof(command->args.report_data.description))) == -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  close(reports_dat);
}

// TODO: Implement these
void execute_add(COMMAND *command, int argc, char **argv) {
  // Create district directory and change the mode if it exists
  if (mkdir(argv[0], 0750)) {
    if (errno == EEXIST) {
      chmod(argv[0], 0750);
    } else {
      perror("Failed to create district directory");
    }
  }

  char path[256];
  snprintf(path, 255, "%s/reports.dat", argv[0]);
  create_file(path, 0664);

  snprintf(path, 255, "%s/district.cfg", argv[0]);
  create_file(path, 0640);

  snprintf(path, 255, "%s/logged_district", argv[0]);
  create_file(path, 0644);

  get_report_data(command);

  if (check_file_permission(command, argv[0])) {
    write_report(command, argv[0]);
  } else {
    fprintf(stderr, "You do not have permissions to write to this file\n");
  }
}

void execute_list(COMMAND *command, int argc, char **argv) {}

void execute_view(COMMAND *command, int argc, char **argv) {}

void execute_remove_report(COMMAND *command, int argc, char **argv) {}

void execute_add_report(COMMAND *command, int argc, char **argv) {}

void execute_update_treshold(COMMAND *command, int argc, char **argv) {}

void execute_filter(COMMAND *command, int argc, char **argv) {}

// these argv start right after the "--command"; argc is smaller as well.
void execute(COMMAND *command, int argc, char **argv) {
  // TODO: Check role using chmod in each case.
  switch (command->type) {
  case ADD:
    // Create the directory and files, then ask for the first report. Subsequent
    // calls on the same district should exit, since we do not have a report id
    // (first report gets the id 1 by default)
    if (argc != 1) {
      fprintf(stderr, "Invalid argument count for the ADD command\n");
      exit(-1);
    }

    execute_add(command, argc, argv);
    break;

  case LIST:
    if (argc != 1) {
      fprintf(stderr, "Invalid argument count for the LIST command\n");
      exit(-1);
    }
    execute_list(command, argc, argv);
    break;

  case VIEW:
    if (argc != 2) {
      fprintf(stderr, "Invalid argument count for the VIEW command\n");
      exit(-1);
    }
    execute_view(command, argc, argv);
    break;

  case REMOVE_REPORT:
    if (argc != 2) {
      fprintf(stderr, "Invalid argument count for the REMOVE_REPORT command\n");
      exit(-1);
    }
    execute_remove_report(command, argc, argv);
    break;

  case UPDATE_TRESHOLD:
    if (argc != 2) {
      fprintf(stderr,
              "Invalid argument count for the UPDATE_TRESHOLD command\n");
      exit(-1);
    }
    execute_update_treshold(command, argc, argv);
    break;

  case FILTER:
    if (argc != 2) {
      fprintf(stderr, "Invalid argument count for the FILTER command\n");
      exit(-1);
    }
    execute_filter(command, argc, argv);
    break;
  }
}
