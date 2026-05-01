#include "../../include/command.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void execute_add(COMMAND *command) {
  // Create district directory and change the mode if it exists
  if (mkdir(command->district, 0750)) {
    if (errno == EEXIST) {
      chmod(command->district, 0750);
    } else {
      perror("Failed to create district directory");
    }
  }

  create_file(command, "reports.dat", 0664);
  create_file(command, "district.cfg", 0640);
  create_file(command, "logged_district", 0644);

  char target_path[256];
  char link_path[256];
  sprintf(target_path, "./%s/reports.dat", command->district);
  sprintf(link_path, "./active_reports-%s", command->district);

  create_symlink(target_path, link_path);

  get_report_data(command);

  if (check_file_permission(command, "reports.dat", "-w-")) {
    write_report(command);
  } else {
    fprintf(stderr, "You do not have permissions to write to this file\n");
  }

  if (check_file_permission(command, "district.cfg", "-w-")) {
    // 3 is the default severity level
    write_district_cfg(command, "severity_level", "3");
  } else {
    fprintf(stderr, "You do not have permissions to write to this file\n");
  }

  if (check_file_permission(command, "logged_district", "-w-")) {
    write_logged_district(command);
  } else {
    fprintf(stderr, "You do not have permissions to write to this file\n");
  }
}

void execute_list(COMMAND *command) {

  REPORT_DATA data = {0};
  int i = 0;

  while (get_report_by_offset(command, i * sizeof(REPORT_DATA), &data) != -1) {
    print_report(data);
    printf("\n");
    i++;
  }

  print_reports_file_info(command);
}

void execute_view(COMMAND *command) {
  if (!check_file_permission(command, "reports.dat", "r")) {
    fprintf(stderr, "You do not have permissions to read reports.dat\n");
    return;
  }

  REPORT_DATA data = {0};
  int id = get_report_by_id(command, command->argv[0], &data);

  if (id != -1) {
    print_report(data);
  } else {
    fprintf(stderr, "Report not found\n");
  }
}

void execute_remove_report(COMMAND *command) {
  if (command->role != MANAGER) {
    fprintf(stderr, "Access denied: Only managers can remove reports.\n");
    return;
  }

  char *report_id = command->argv[0];
  REPORT_DATA data;

  off_t to_delete_from_offset = get_report_by_id(command, report_id, &data);
  if (to_delete_from_offset != -1) {
    delete_report_from_offset(command, to_delete_from_offset);
  }
}

void execute_update_threshold(COMMAND *command) {
  if (command->role != MANAGER) {
    fprintf(stderr, "Access denied: Only managers can update thresholds.\n");
    return;
  }

  update_parameter(command, "severity_level", command->argv[0]);
}

void execute_filter(COMMAND *command) {
  REPORT_DATA data;
  int i = 0;

  char field[MAX_FIELD_LENGTH];
  char op[MAX_OP_LENGTH];
  char value[MAX_VAL_LENGTH];

  while (get_report_by_offset(command, i * sizeof(REPORT_DATA), &data) != -1) {
    int conditions_met = 1;

    for (int i = 0; i < command->argc; i++) {
      parse_condition(command->argv[i], field, op, value);

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

void execute_remove_district(COMMAND *command) {
  if (command->role != MANAGER) {
    fprintf(stderr, "You do not have permissions for this command\n");
    exit(-1);
  }

  // Path traversal
  if (strstr(command->district, "../") || strchr(command->district, '/')) {
    fprintf(stderr, "Invalid path\n");
    exit(-2);
  }

  char symlink_path[256];
  sprintf(symlink_path, "./active_reports-%s", command->district);

  char *rm_args[4] = {"rm", "-rf", command->district, 0};

  pid_t pid = fork();

  if (pid == 0) { // Child process
    unlink(symlink_path);
    execvp("rm", rm_args);
  } else if (pid == -1) {
    fprintf(stderr, "Could not fork the remove_district process\n");
    exit(-1);
  } else { // Parent process
    waitpid(pid, NULL, 0);
  }
}

// these argv start right after the "--command"; argc is smaller as well.
void execute(COMMAND *command) {
  switch (command->type) {
  case ADD:
    // Create the directory and files, then ask for the first report.
    // Subsequent calls on the same district should exit, since we do not have
    // a report id (first report gets the id 1 by default)
    if (command->argc != 0) {
      fprintf(stderr, "Invalid argument count for the ADD command\n");
      exit(-1);
    }

    execute_add(command);
    break;

  case LIST:
    if (command->argc != 0) {
      fprintf(stderr, "Invalid argument count for the LIST command\n");
      exit(-1);
    }
    execute_list(command);
    break;

  case VIEW:
    if (command->argc != 1) {
      fprintf(stderr, "Invalid argument count for the VIEW command\n");
      exit(-1);
    }
    execute_view(command);
    break;

  case REMOVE_REPORT:
    if (command->argc != 1) {
      fprintf(stderr, "Invalid argument count for the REMOVE_REPORT command\n");
      exit(-1);
    }
    execute_remove_report(command);
    break;

  case UPDATE_THRESHOLD:
    if (command->argc != 1) {
      fprintf(stderr,
              "Invalid argument count for the UPDATE_THRESHOLD command\n");
      exit(-1);
    }
    execute_update_threshold(command);
    break;

  case FILTER:
    if (command->argc < 1) {
      fprintf(stderr, "Invalid argument count for the FILTER command\n");
      exit(-1);
    }
    execute_filter(command);
    break;

  case REMOVE_DISTRICT:
    if (command->argc != 0) {
      fprintf(stderr,
              "Invalid argument count for the REMVOE_DISTRICT command\n");
      exit(-1);
    }
    execute_remove_district(command);
    break;
  }

  if (command->type != REMOVE_DISTRICT) {
    write_logged_district(command);
  }
}
