#include "../../include/file_utils.h"

void create_file(COMMAND *command, char *file, mode_t mode) {
  char pathname[256];
  if (strlen(command->district) + strlen(file) >= 256) {
    fprintf(stderr, "Pathname is too long\n");
  }

  strcpy(pathname, command->district);
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
int check_file_permission(COMMAND *command, char *file, char *permissions) {
  char path[256];
  sprintf(path, "%s/%s", command->district, file);
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

int open_file(COMMAND *command, char *file, char *mode, int flags) {
  char target_path[256];
  int is_report = (strcmp(file, "reports.dat") == 0);

  if (is_report) {
    sprintf(target_path, "./active_reports-%s", command->district);
    if (check_symlink(target_path) == -1) {
      return -1;
    }
  } else {
    sprintf(target_path, "./%s/%s", command->district, file);
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

void create_symlink(const char *target, const char *linkpath) {
  if (symlink(target, linkpath) != 0) {
    if (errno == EEXIST) {
      return;
    }

    perror("Symlink creation error");
    exit(-1);
  }
}
