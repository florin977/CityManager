#include "../include/command.h"
#include <asm-generic/errno-base.h>
#include <bits/pthreadtypes.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
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

// -1 if file is missing, 0 on success
int check_symlink(const char *filepath) {
  struct stat link_stat;
  struct stat target_stat;

  if (lstat(filepath, &link_stat) == 0) {
    if (S_ISLNK(link_stat.st_mode)) {
      if (stat(filepath, &target_stat) != 0) {
        if (errno == ENOENT) {
          fprintf(
              stderr,
              "WARNING: Dangling link detected at '%s'. Target is missing.\n",
              filepath);
          return -1;
        } else {
          perror("Stat failed for an unexpected reason");
          return -1;
        }
      }
    }
  }

  return 0;
}

int open_file(COMMAND *command, char *district, char *file, char *mode,
              int flags) {
  char target_path[256];
  int is_report = (strcmp(file, "reports.dat") == 0);

  if (is_report) {
    sprintf(target_path, "./active_reports-%s", district);
    if (check_symlink(target_path) == -1) {
      return -1;
    }
  } else {
    sprintf(target_path, "./%s/%s", district, file);
  }

  int fd = -1;

  if (mode[0] == 'r' && mode[1] == 'w') {
    flags |= O_RDWR;
  } else if (mode[0] == 'r') {
    flags |= O_RDONLY;
  } else if (mode[1] == 'w') {
    flags |= O_WRONLY;
  }

  if ((fd = open(target_path, flags)) == -1) {
    perror(target_path);
    return -1;
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
// report id on successful read, -1 on EOF or anything else.
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

  return data->report_id;
}

// offset on success, -1 on failure or not found
__off_t get_report_by_id(COMMAND *command, char *district, char *report_id,
                         REPORT_DATA *data) {
  int id = atoi(report_id);
  int id2 = -1;
  int i = 0;

  while ((id2 = get_report_by_offset(command, district, i * sizeof(REPORT_DATA),
                                     data)) != -1) {
    if (id2 == id) {
      return (__off_t)i * sizeof(REPORT_DATA);
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

void write_district_cfg(COMMAND *command, char *district, char *field,
                        char *value) {
  int district_cfg = open_file(command, district, "district.cfg", "-w", 0);

  dprintf(district_cfg, "%s=%s\n", field, value);

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

  // Copy all reports after this one
  lseek(reports_dat, offset + sizeof(REPORT_DATA), SEEK_SET);

  read(reports_dat, buffer, copy_size);

  // Place write cursor at the begining of the report needed to be deleted
  lseek(reports_dat, offset, SEEK_SET);

  write(reports_dat, buffer, copy_size);

  // Delete the extra bytes
  ftruncate(reports_dat, file_end - sizeof(REPORT_DATA));

  free(buffer);
  close(reports_dat);
}

void update_parameter(COMMAND *command, char *district, char *parameter,
                      char *value) {
  int district_cfg = open_file(command, district, "district.cfg", "rw-", 0);
  int file_size = lseek(district_cfg, 0, SEEK_END);
  lseek(district_cfg, 0, SEEK_SET);

  char *buffer = NULL;
  if ((buffer = malloc((file_size + 1) * sizeof(char))) == NULL) {
    perror("Malloc");
    exit(-1);
  }

  buffer[file_size] = 0;

  if (read(district_cfg, buffer, file_size) != file_size) {
    fprintf(stderr, "Error while reading district.cfg\n");
    free(buffer);
    exit(-1);
  }

  char *new_line = NULL;
  if ((new_line = malloc((strlen(parameter) + strlen(value) + 2) *
                         sizeof(char))) == NULL) {
    perror("Malloc");
    free(buffer);
    exit(-1);
  }
  sprintf(new_line, "%s=%s\n", parameter, value);

  char *match = strstr(buffer, parameter);

  if (match != NULL) {
    char *line_start = match;
    char *line_end = match;

    // Start of the line with the parameter on it
    while (line_start > buffer && *(line_start - 1) != '\n') {
      line_start--;
    }

    // End of the lien with the parameter on it
    while (*(line_end) != '\n' && *(line_end) != 0) {
      line_end++;
    }
    // Jump over \n
    line_end++;

    __off_t start_offset = line_start - buffer;
    __off_t copy_length = file_size - (line_end - buffer);

    // Overwrite the line with the parameter
    lseek(district_cfg, start_offset, SEEK_SET);
    write(district_cfg, new_line, strlen(new_line));
    // Append the other content.
    write(district_cfg, line_end, copy_length);

    __off_t new_size = start_offset + strlen(new_line) + copy_length;

    ftruncate(district_cfg, new_size);
  }

  free(new_line);
  free(buffer);
}

/**
 * Helper function to copy characters while resolving escape sequences.
 * Converts "\:" to ":" and "\\" to "\".
 */
static void copy_unescaped(char *dest, const char *start, size_t raw_len) {
  size_t i = 0;
  size_t j = 0;

  while (i < raw_len) {
    if (start[i] == '\\' && i + 1 < raw_len &&
        (start[i + 1] == ':' || start[i + 1] == '\\')) {
      dest[j++] = start[i + 1];
      i += 2;
    } else {
      dest[j++] = start[i++];
    }
  }
  dest[j] = '\0'; // Null-terminate the destination
}

/**
 * Parses a string formatted as "field:op:value" into its three components.
 * Checks all lengths against MAX limits, ensuring space for null terminators.
 */
int parse_condition(const char *input, char *field, char *op, char *value) {
  if (!input || !field || !op || !value) {
    return -1;
  }

  const char *p = input;
  const char *colon1 = NULL;
  const char *colon2 = NULL;

  // 1. Scan for unescaped colons
  while (*p) {
    if (*p == '\\') {
      if (*(p + 1) == ':' || *(p + 1) == '\\') {
        p += 2;
        continue;
      }
    } else if (*p == ':') {
      if (!colon1) {
        colon1 = p;
      } else if (!colon2) {
        colon2 = p;
        break;
      }
    }
    p++;
  }

  if (!colon1 || !colon2) {
    return -1;
  }

  // 2. Calculate raw lengths
  size_t raw_field_len = colon1 - input;
  size_t raw_op_len = colon2 - (colon1 + 1);
  size_t raw_val_len = strlen(colon2 + 1);

  // 3. Strict length checks
  // The raw length must be strictly less than the max size to leave
  // room for the null terminator.
  if (raw_field_len >= MAX_FIELD_LENGTH || raw_op_len >= MAX_OP_LENGTH ||
      raw_val_len >= MAX_VAL_LENGTH) {
    return -1;
  }

  // 4. Extract and unescape the components
  copy_unescaped(field, input, raw_field_len);
  copy_unescaped(op, colon1 + 1, raw_op_len);
  copy_unescaped(value, colon2 + 1, raw_val_len);

  return 0; // Success
}

/**
 * Compares two long long integers based on a string operator.
 */
static int compare_numeric(long long left, long long right, const char *op) {
  if (strcmp(op, "==") == 0)
    return left == right;
  if (strcmp(op, "!=") == 0)
    return left != right;
  if (strcmp(op, "<") == 0)
    return left < right;
  if (strcmp(op, "<=") == 0)
    return left <= right;
  if (strcmp(op, ">") == 0)
    return left > right;
  if (strcmp(op, ">=") == 0)
    return left >= right;
  return 0; // Unknown operator
}

/**
 * Compares two strings using strcmp based on a string operator.
 */
static int compare_string(const char *left, const char *right, const char *op) {
  int cmp = strcmp(left, right);
  if (strcmp(op, "==") == 0)
    return cmp == 0;
  if (strcmp(op, "!=") == 0)
    return cmp != 0;
  if (strcmp(op, "<") == 0)
    return cmp < 0;
  if (strcmp(op, "<=") == 0)
    return cmp <= 0;
  if (strcmp(op, ">") == 0)
    return cmp > 0;
  if (strcmp(op, ">=") == 0)
    return cmp >= 0;
  return 0; // Unknown operator
}

/**
 * Parses a string into a time_t. Supports ISO 8601 (YYYY-MM-DDTHH:MM:SS)
 * or raw Unix epoch integers.
 */
static time_t parse_time(const char *value) {
  struct tm t = {0};

  // Try to parse as ISO 8601 Date/Time string
  if (sscanf(value, "%d-%d-%dT%d:%d:%d", &t.tm_year, &t.tm_mon, &t.tm_mday,
             &t.tm_hour, &t.tm_min, &t.tm_sec) == 6) {

    t.tm_year -= 1900; // tm_year is years since 1900
    t.tm_mon -= 1;     // tm_mon is 0-11
    t.tm_isdst = -1;   // Let mktime determine Daylight Saving Time

    time_t parsed_time = mktime(&t);
    if (parsed_time != (time_t)-1) {
      return parsed_time;
    }
  }

  // Fallback: Try parsing as a raw integer (Unix Epoch)
  char *endptr;
  long long epoch = strtoll(value, &endptr, 10);
  if (*value != '\0' && *endptr == '\0') {
    return (time_t)epoch;
  }

  return (time_t)-1; // Parsing completely failed
}

// --- Main Function ---

/**
 * Evaluates whether a REPORT_DATA record satisfies a given condition.
 * @return 1 if condition is met, 0 if not met or on error (invalid
 * field/conversion).
 */
int match_condition(REPORT_DATA *r, const char *field, const char *op,
                    const char *value) {
  if (!r || !field || !op || !value) {
    return 0;
  }

  // 1. Handle "severity" (Integer)
  if (strcmp(field, "severity") == 0) {
    char *endptr;
    long sev_val = strtol(value, &endptr, 10);

    // Check if conversion succeeded entirely (no trailing characters)
    if (*value == '\0' || *endptr != '\0') {
      return 0;
    }
    return compare_numeric((long long)r->severity_level, (long long)sev_val,
                           op);
  }

  // 2. Handle "timestamp" (time_t)
  else if (strcmp(field, "timestamp") == 0) {
    time_t time_val = parse_time(value);
    if (time_val == (time_t)-1) {
      return 0; // Conversion failed
    }
    return compare_numeric((long long)r->timestamp, (long long)time_val, op);
  }

  // 3. Handle "category" (String)
  else if (strcmp(field, "category") == 0) {
    return compare_string(r->issue_category, value, op);
  }

  // 4. Handle "inspector" (String)
  else if (strcmp(field, "inspector") == 0) {
    return compare_string(r->username, value, op);
  }

  // Unknown field
  return 0;
}

void create_symlink(const char *target, const char *linkpath) {
  if (symlink(target, linkpath) != 0) {
    if (errno == EEXIST) {
      return;
    }

    perror("Symlink creation error");
    exit(-1);
  }
}

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

  char target_path[256];
  char link_path[256];
  sprintf(target_path, "./%s/reports.dat", argv[0]);
  sprintf(link_path, "./active_reports-%s", argv[0]);

  create_symlink(target_path, link_path);

  get_report_data(command, argv[0]);

  if (check_file_permission(command, argv[0], "reports.dat", "-w-")) {
    write_report(command, argv[0]);
  } else {
    fprintf(stderr, "You do not have permissions to write to this file\n");
  }

  if (check_file_permission(command, argv[0], "district.cfg", "-w-")) {
    // 3 is the default severity level
    write_district_cfg(command, argv[0], "severity_level", "3");
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

void execute_update_threshold(COMMAND *command, char **argv) {
  update_parameter(command, argv[0], "severity_level", argv[1]);
}

void execute_filter(COMMAND *command, int argc, char **argv) {
  REPORT_DATA data;
  int i = 0;

  char field[MAX_FIELD_LENGTH];
  char op[MAX_OP_LENGTH];
  char value[MAX_VAL_LENGTH];

  while (get_report_by_offset(command, argv[0], i * sizeof(REPORT_DATA),
                              &data) != -1) {
    int conditions_met = 1;

    for (int i = 1; i < argc; i++) {
      parse_condition(argv[i], field, op, value);

      if (!match_condition(&data, field, op, value)) {
        conditions_met = 0;
        break;
      }
    }

    if (conditions_met) {
      print_report(data);
    }

    i++;
  }
}

// these argv start right after the "--command"; argc is smaller as well.
void execute(COMMAND *command, int argc, char **argv) {
  switch (command->type) {
  case ADD:
    // Create the directory and files, then ask for the first report.
    // Subsequent calls on the same district should exit, since we do not have
    // a report id (first report gets the id 1 by default)
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
    if (argc < 2) {
      fprintf(stderr, "Invalid argument count for the FILTER command\n");
      exit(-1);
    }
    execute_filter(command, argc, argv);
    break;
  }
}
