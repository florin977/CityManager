#include "../include/command.h"
#include "../include/sanitization.h"

int main(int argc, char **argv) {
  if (argc < 6) {
    fprintf(stderr, "Invalid argument count! Usage: city_manager --role "
                    "<inspector|manager> --user <user name> --<command> "
                    "<...command details..>\n");
    exit(-1);
  }

  if (!check_arg_integrity(argv)) {
    fprintf(stderr, "Invalid argument usage! Usage: city_manager --role "
                    "<inspector|manager> --user <user name> --<command> "
                    "<...command details..>\n");
    exit(-1);
  }

  // Fill the struct with 0s, else the write using sizeof() in write_report will
  // cause a stack memory leak and show actual stack pointers, leading to the
  // same vulnerability the heartbleed bug used.
  COMMAND command = {0};

  get_role(&command, argv[2]);
  get_permissions(&command);
  get_username(&command, argv[4]);
  get_type(&command, argv[5]);
  get_args(&command, argc, argv);
  execute(&command);

  return 0;
}
