#include "../include/command.h"
#include "../include/sanitization.h"

int main(int argc, char **argv) {
  if (argc < 6) {
    fprintf(stderr, "Invalid argument count! Usage: city_manager --role "
                    "<inspector|manager> --user <user name> --<command> "
                    "<...command details..>\n");
    exit(-1);
  }

  if (!check_arg_integrity(argc, argv)) {
    fprintf(stderr, "Invalid argument usage! Usage: city_manager --role "
                    "<inspector|manager> --user <user name> --<command> "
                    "<...command details..>\n");
    exit(-1);
  }

  COMMAND command;
  get_role(&command, argv[2]);
  get_permissions(&command);
  get_username(&command, argv[4]);
  get_type(&command, argv[5]);
  execute(&command, argc - 6, &argv[6]);

  return 0;
}
