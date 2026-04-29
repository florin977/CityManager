#include "../include/cfg_logs_utils.h"

void update_parameter(COMMAND *command, char *parameter, char *value) {
  int district_cfg = open_file(command, "district.cfg", "rw-", 0);

  struct stat sb;
  if (fstat(district_cfg, &sb) == -1) {
    perror("fstat failed on district.cfg");
    close(district_cfg);
    return;
  }

  mode_t current_perms = sb.st_mode & 0777;
  if (current_perms != 0640) {
    fprintf(stderr,
            "Diagnostic error: permissions for district.cfg have been altered! "
            "Expected 0640, found 0%o. Aborting.\n",
            current_perms);
    close(district_cfg);
    return;
  }

  off_t file_size = sb.st_size;
  lseek(district_cfg, 0, SEEK_SET);

  char *buffer = NULL;
  if ((buffer = malloc((file_size + 1) * sizeof(char))) == NULL) {
    perror("Malloc");
    exit(-1);
  }

  buffer[file_size] = 0;

  if (read(district_cfg, buffer, file_size) != file_size) {
    fprintf(stderr, "Error while reading district.cfg\n");
    close(district_cfg);
    free(buffer);
    exit(-1);
  }

  char *new_line = NULL;
  if ((new_line = malloc((strlen(parameter) + strlen(value) + 2) *
                         sizeof(char))) == NULL) {
    perror("Malloc");
    close(district_cfg);
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

    // End of the line with the parameter on it
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

void write_district_cfg(COMMAND *command, char *field, char *value) {
  int district_cfg = open_file(command, "district.cfg", "-w", 0);

  dprintf(district_cfg, "%s=%s\n", field, value);

  close(district_cfg);
}

void write_logged_district(COMMAND *command) {
  int logged_district = open_file(command, "logged_district", "-w", O_APPEND);

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

  dprintf(logged_district, "%lld\t%30s\t%11s\t%16s\n", (long long)time(NULL),
          command->username, role, cmd_type);

  close(logged_district);
}
