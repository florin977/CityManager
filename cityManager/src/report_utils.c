#include "../include/report_utils.h"

void print_reports_file_info(COMMAND *command) {
  char path[256];
  sprintf(path, "%s/reports.dat", command->district);

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

int get_report_id(COMMAND *command) {
  int reports_dat = open_file(command, "reports.dat", "r-", O_APPEND);
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

void write_report(COMMAND *command) {
  // Open the reports.dat file (-1 on error)
  int reports_dat = open_file(command, "reports.dat", "-w", O_APPEND);

  // Dump the entire struct in reports.dat (in binary)
  if ((write(reports_dat, &command->report_data, sizeof(REPORT_DATA))) == -1) {
    perror("Writing to reports.dat");
    exit(-1);
  }

  close(reports_dat);
}

// Report id on successful read, -1 on EOF or anything else.
int get_report_by_offset(COMMAND *command, off_t offset, REPORT_DATA *data) {
  int reports_dat = open_file(command, "reports.dat", "r--", 0);

  if (reports_dat == -1) {
    return -1;
  }

  if (!check_file_permission(command, "reports.dat", "r--")) {
    fprintf(stderr, "Access denied: Cannot read reports.dat\n");
    close(reports_dat);
    return -1;
  }

  if (lseek(reports_dat, offset, SEEK_SET) == -1) {
    close(reports_dat);
    return -1;
  }

  if (read(reports_dat, data, sizeof(REPORT_DATA)) != sizeof(REPORT_DATA)) {
    close(reports_dat);
    return -1;
  }

  close(reports_dat);

  return data->report_id;
}

// offset on success, -1 on failure or not found
off_t get_report_by_id(COMMAND *command, char *report_id, REPORT_DATA *data) {
  int id = atoi(report_id);
  int id2 = -1;
  int i = 0;

  while ((id2 = get_report_by_offset(command, i * sizeof(REPORT_DATA), data)) !=
         -1) {
    if (id2 == id) {
      return (off_t)i * sizeof(REPORT_DATA);
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

void get_report_data(COMMAND *command) {
  command->report_data.report_id = get_report_id(command);

  printf("%d\n", command->report_data.report_id);
  printf("Please enter the report data:\nX: ");
  if (scanf("%f", &command->report_data.coords.lat) != 1) {
    fprintf(stderr, "Invalid latitude\n");
    exit(-1);
  }

  printf("Y: ");
  if (scanf("%f", &command->report_data.coords.lng) != 1) {
    fprintf(stderr, "Invalid longitutde\n");
    exit(-1);
  }

  printf("Category (road/lighting/flooding/other): ");
  if (scanf("%29s", command->report_data.issue_category) != 1) {
    fprintf(stderr, "Invalid category\n");
    exit(-1);
  }

  printf("Severity lvel(1/2/3): ");
  if (scanf("%d", &command->report_data.severity_level) != 1) {
    fprintf(stderr, "Invalid severity level\n");
    exit(-1);
  }
  // Consume newline
  fgetc(stdin);

  // Get the timestamp
  command->report_data.timestamp = time(NULL);

  printf("Description: ");
  if (fgets(command->report_data.description, 200, stdin) == NULL) {
    fprintf(stderr, "Invalid description\n");
    exit(-1);
  }

  // get rid of the newline at the end
  command->report_data
      .description[strlen(command->report_data.description) - 1] = 0;

  strcpy(command->report_data.username, command->username);
}

void delete_report_from_offset(COMMAND *command, off_t offset) {
  int reports_dat = open_file(command, "reports.dat", "rw-", 0);

  if (reports_dat == -1) {
    return;
  }

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
