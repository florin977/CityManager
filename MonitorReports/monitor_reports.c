#include <asm-generic/errno-base.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t keep_alive = 1;

void handle_sigint(int sig) {
  keep_alive = 0;

  char msg[1024];
  snprintf(msg, 1024,
           "Monitor_pid: SIGINT received, stopping the program...\n");
  fprintf(stdout, "%04ld%s", strlen(msg), msg);
  fflush(stdout);
}

void handle_sigusr1(int sig) {
  char msg[1024];
  snprintf(msg, 1024, "Monitor_pid: A new report has been added.\n");
  fprintf(stdout, "%04ld%s", strlen(msg), msg);
  fflush(stdout);
}

pid_t get_monitor_pid() {
  int fd = open(".monitor_pid", O_RDONLY);

  if (fd == -1) {
    return -1;
  }

  char buffer[100];
  off_t file_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  if (read(fd, buffer, file_size) == -1) {
    return -1;
  }

  pid_t pid = -1;
  pid = strtol(buffer, NULL, 10);

  return pid;
}

void start() {
  pid_t monitor_pid = getpid();
  char pid[100];
  snprintf(pid, 100, "%d", monitor_pid);
  int fd = -1;

  if ((fd = open(".monitor_pid", O_CREAT | O_EXCL | O_RDWR | O_TRUNC, 0777)) ==
      -1) {
    if (errno == EEXIST) {
      int monitor_pid = get_monitor_pid();
      char msg[1024];
      snprintf(msg, 1024, "Monitor reports already running. PID is: %d\n",
               monitor_pid);

      fprintf(stdout, "%04ld%s", strlen(msg), msg);
      fflush(stdout);
      exit(-1);
    }
  }

  write(fd, pid, strlen(pid));
  close(fd);
}

void stop() {
  if (unlink(".monitor_pid") == -1) {
    perror("Failed to delete .monitor_pid");
  } else {
    char msg[1024];
    snprintf(msg, 1024, "Monitor_pid ended successfully.\n");
    fprintf(stdout, "%04ld%s", strlen(msg), msg);
    fflush(stdout);
  }
}

int main(int argc, char **argv) {
  struct sigaction sigint, sigusr1;

  sigemptyset(&sigint.sa_mask);
  sigint.sa_handler = handle_sigint;
  sigint.sa_flags = 0;
  sigaction(SIGINT, &sigint, NULL);

  sigemptyset(&sigusr1.sa_mask);
  sigusr1.sa_handler = handle_sigusr1;
  sigusr1.sa_flags = 0;
  sigaction(SIGUSR1, &sigusr1, NULL);

  start();

  while (keep_alive) {
    pause();
  }

  stop();

  return 0;
}
