#include "../include/sanitization.h"

int check_command_integrity(char **argv) {
  return (strcmp(argv[5], "add") || strcmp(argv[5], "list") ||
          strcmp(argv[5], "view") || strcmp(argv[5], "remove_report") ||
          strcmp(argv[5], "add_report") ||
          strcmp(argv[5], "update_threshold") || strcmp(argv[5], "filter"));
}

int check_arg_integrity(char **argv) {
  int valid_command = check_command_integrity(argv);

  return (!strcmp(argv[1], "--role") && !strcmp(argv[3], "--user") &&
          valid_command);
}
