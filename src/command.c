#include "../include/command.h"
#include <bits/pthreadtypes.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

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

void create_file(char *district, char *file, mode_t mode) {
  char pathname[256];
  if (strlen(district) + strlen(file) >= 256) {
    fprintf(stderr, "Pathname is too long\n");
  }

  strcpy(pathname, district);
  strcat(pathname, "/");
  strcat(pathname, file);

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

// format: rwx, -w- and so on
int check_file_permission(COMMAND *command, char *district, char *file,
                          char *permissions) {
  char path[256];
  sprintf(path, "%s/%s", district, file);
  struct stat sb;
  stat(path, &sb);
  int r = permissions[0] == 'r' ? sb.st_mode & command->permission.READ_BIT : 1;
  int w =
      permissions[1] == 'w' ? sb.st_mode & command->permission.WRITE_BIT : 1;
  int x =
      permissions[2] == 'x' ? sb.st_mode & command->permission.EXECUTE_BIT : 1;
  return (r && w && x);
}

int open_file(COMMAND *command, char *district, char *file, char *mode,
              int flags) {
  int fd = -1;
  char path[256];

  if (mode[0] == 'r' && mode[1] == 'w') {
    flags |= O_RDWR;
  } else if (mode[0] == 'r') {
    flags |= O_RDONLY;
  } else if (mode[1] == 'w') {
    flags |= O_WRONLY;
  }

  sprintf(path, "%s/%s", district, file);

  if ((fd = open(path, flags)) == -1) {
    perror(path);
    exit(-1);
  }

  return fd;
}

void print_reports_file_info(COMMAND *command, char *district) {
  char path[256];
  sprintf(path, "%s/reports.dat", district);

  struct stat sb;
  stat(path, &sb);
  mode_t mode = sb.st_mode;

  char permissions[10];

  permissions[0] = (mode & S_IRUSR) ? 'r' : '-';
  permissions[1] = (mode & S_IWUSR) ? 'w' : '-';
  permissions[2] = (mode & S_IXUSR) ? 'x' : '-';

  permissions[3] = (mode & S_IRGRP) ? 'r' : '-';
  permissions[4] = (mode & S_IWGRP) ? 'w' : '-';
  permissions[5] = (mode & S_IXGRP) ? 'x' : '-';

  permissions[6] = (mode & S_IROTH) ? 'r' : '-';
  permissions[7] = (mode & S_IWOTH) ? 'w' : '-';
  permissions[8] = (mode & S_IXOTH) ? 'x' : '-';
  permissions[9] = 0;

  char time[50];
  struct tm *time_info = localtime(&sb.st_mtime);
  strftime(time, 49, "%b %d %H:%M", time_info);

  printf("%s %ld %s %s\n", permissions, sb.st_size, time, "reports.dat");
}

int get_report_id(COMMAND *command, char *district) {
  int reports_dat = open_file(command, district, "reports.dat", "r-", O_APPEND);
  REPORT_DATA data;

  int offset = lseek(reports_dat, -sizeof(REPORT_DATA), SEEK_END);

  if (offset == -1) {
    return 0;
  }

  if (read(reports_dat, &data, sizeof(REPORT_DATA)) != sizeof(REPORT_DATA)) {
    return 0;
  }

  return data.report_id + 1;
}

void write_report(COMMAND *command, char *district) {
  // Open the reports.dat file (-1 on error)
  int reports_dat = open_file(command, district, "reports.dat", "-w", O_APPEND);

  // Dump the entire struct in reports.dat (in binary)
  if ((write(reports_dat, &command->args.report_data, sizeof(REPORT_DATA))) ==
      -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  close(reports_dat);
}
// 1 on successful read, -1 on EOF or anything else.
int get_report_by_offset(COMMAND *command, char *district, __off_t offset,
                         REPORT_DATA *data) {
  int reports_dat =
      open_file(command, district, "reports.dat", "r--", O_APPEND);

  if (check_file_permission(command, district, "reports.dat", "r--")) {
    lseek(reports_dat, offset, SEEK_SET);

    if (read(reports_dat, data, sizeof(REPORT_DATA)) != sizeof(REPORT_DATA)) {
      return -1;
    }

  } else {
    fprintf(stderr, "Can not read reports.dat\n");
  }

  close(reports_dat);

  return 1;
}

// offset on success, -1 on failure or not found
__off_t get_report_by_id(COMMAND *command, char *district, char *report_id,
                         REPORT_DATA *data) {
  int id = atoi(report_id);
  int i = 0;

  while (get_report_by_offset(command, district, i * sizeof(REPORT_DATA),
                              data) != -1) {
    if (data->report_id == id) {
      return i * sizeof(REPORT_DATA);
    }

    i++;
  }

  return -1;
}

void print_report(REPORT_DATA data) {
  printf("ID: %d\nInspector: %s\nGPS coordinates: (%g,%g)\nIssue category: "
         "%s\nSeverity level: %d\nTimestamp: %lld\nDescription: %s\n",
         data.report_id, data.username, data.coords.lat, data.coords.lng,
         data.issue_category, data.severity_level, (long long)data.timestamp,
         data.description);
}

void write_district_cfg(COMMAND *command, char *district, int severity_level) {
  int district_cfg =
      open_file(command, district, "district.cfg", "-w", O_APPEND);

  dprintf(district_cfg, "severity level=%d\n", severity_level);

  close(district_cfg);
}

void write_logged_district(COMMAND *command, char *district) {
  int logged_district =
      open_file(command, district, "logged_district", "-w", O_APPEND);

  char role[10];
  switch (command->role) {
  case MANAGER:
    strcpy(role, "manager");
    break;

  case INSPECTOR:
    strcpy(role, "inspector");
    break;
  }

  char cmd_type[17];
  switch (command->type) {
  case ADD:
    strcpy(cmd_type, "add");
    break;

  case LIST:
    strcpy(cmd_type, "list");
    break;

  case VIEW:
    strcpy(cmd_type, "view");
    break;

  case REMOVE_REPORT:
    strcpy(cmd_type, "remove_report");
    break;

  case UPDATE_THRESHOLD:
    strcpy(cmd_type, "update_threshold");
    break;

  case FILTER:
    strcpy(cmd_type, "filter");
    break;
  }

  dprintf(logged_district, "%lld\t%30s\t%11s\t%16s/n", (long long)time(NULL),
          command->username, role, cmd_type);

  close(logged_district);
}

void get_report_data(COMMAND *command, char *district) {
  command->args.report_data.report_id = get_report_id(command, district);

  printf("%d\n", command->args.report_data.report_id);
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

  // Get the timestamp
  command->args.report_data.timestamp = time(NULL);

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

void delete_report_from_offset(COMMAND *command, char *district, off_t offset) {
  int reports_dat = open_file(command, district, "reports.dat", "rw-", 0);

  // Get file size in bytes
  off_t file_end = lseek(reports_dat, 0, SEEK_END);
  // Data that needs to be shifted
  size_t copy_size = (size_t)file_end - (offset + sizeof(REPORT_DATA));
  char *buffer = NULL;

  if ((buffer = malloc(copy_size * sizeof(char))) == NULL) {
    perror("buffer copy malloc");
    exit(-1);
  }

  // Place write cursor at the begining of the report needed to be deleted
  lseek(reports_dat, offset, SEEK_SET);

  write(reports_dat, buffer, copy_size);

  // Delete the extra bytes
  ftruncate(reports_dat, file_end - sizeof(REPORT_DATA));

  free(buffer);
  close(reports_dat);
}

// TODO: Implement these
void execute_add(COMMAND *command, char **argv) {
  // Create district directory and change the mode if it exists
  if (mkdir(argv[0], 0750)) {
    if (errno == EEXIST) {
      chmod(argv[0], 0750);
    } else {
      perror("Failed to create district directory");
    }
  }

  create_file(argv[0], "reports.dat", 0664);
  create_file(argv[0], "district.cfg", 0640);
  create_file(argv[0], "logged_district", 0644);

  get_report_data(command, argv[0]);

  if (check_file_permission(command, argv[0], "reports.dat", "-w-")) {
    write_report(command, argv[0]);
  } else {
    fprintf(stderr, "You do not have permissions to write to this file\n");
  }

  if (check_file_permission(command, argv[0], "district.cfg", "-w-")) {
    // 3 is the default severity level
    write_district_cfg(command, argv[0], 3);
  } else {
    fprintf(stderr, "You do not have permissions to write to this file\n");
  }

  if (check_file_permission(command, argv[0], "logged_district", "-w-")) {
    write_logged_district(command, argv[0]);
  } else {
    fprintf(stderr, "You do not have permissions to write to this file\n");
  }
}

void execute_list(COMMAND *command, char **argv) {

  REPORT_DATA data = {0};
  int i = 0;

  while (get_report_by_offset(command, argv[0], i * sizeof(REPORT_DATA),
                              &data) != -1) {
    print_report(data);
    printf("\n");
    i++;
  }

  print_reports_file_info(command, argv[0]);
}

void execute_view(COMMAND *command, char **argv) {
  if (!check_file_permission(command, argv[0], "reports.dat", "r")) {
    fprintf(stderr, "You do not have permissions to read reports.dat\n");
    return;
  }

  REPORT_DATA data = {0};
  get_report_by_id(command, argv[0], argv[1], &data);

  if (data.report_id != -1) {
    print_report(data);
  } else {
    fprintf(stderr, "Report not found\n");
  }
}

void execute_remove_report(COMMAND *command, char **argv) {
  char *district = argv[0];
  char *report_id = argv[1];
  REPORT_DATA data;

  off_t to_delete_from_offset =
      get_report_by_id(command, district, report_id, &data);
  if (to_delete_from_offset != -1) {
    delete_report_from_offset(command, district, to_delete_from_offset);
  }
}

void execute_update_threshold(COMMAND *command, char **argv) {}

void execute_filter(COMMAND *command, char **argv) {}

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

    execute_add(command, argv);
    break;

  case LIST:
    if (argc != 1) {
      fprintf(stderr, "Invalid argument count for the LIST command\n");
      exit(-1);
    }
    execute_list(command, argv);
    break;

  case VIEW:
    if (argc != 2) {
      fprintf(stderr, "Invalid argument count for the VIEW command\n");
      exit(-1);
    }
    execute_view(command, argv);
    break;

  case REMOVE_REPORT:
    if (argc != 2) {
      fprintf(stderr, "Invalid argument count for the REMOVE_REPORT command\n");
      exit(-1);
    }
    execute_remove_report(command, argv);
    break;

  case UPDATE_THRESHOLD:
    if (argc != 2) {
      fprintf(stderr,
              "Invalid argument count for the UPDATE_THRESHOLD command\n");
      exit(-1);
    }
    execute_update_threshold(command, argv);
    break;

  case FILTER:
    if (argc != 2) {
      fprintf(stderr, "Invalid argument count for the FILTER command\n");
      exit(-1);
    }
    execute_filter(command, argv);
    break;
  }
}
