#include <fcntl.h>
#include <signal.h>
#include <stdio.h>  // For snprintf, perror, fprintf
#include <stdlib.h> // For exit
#include <string.h> // For strlen
#include <unistd.h>

volatile sig_atomic_t keep_alive = 1;

void handle_sigint(int sig) {
  keep_alive = 0;

  const char *message =
      "Monitor_pid: SIGINT received, stopping the program...\n";
  write(STDOUT_FILENO, message, strlen(message));
}

void handle_sigurs1(int sig) {

  const char *message = "Monitor_pid: A new report has been added.\n";
  write(STDOUT_FILENO, message, strlen(message));
}

void start() {
  pid_t monitor_pid = getpid();
  char pid[100];
  snprintf(pid, 100, "%d", monitor_pid);
  int fd = -1;

  if ((fd = open(".monitor_pid", O_CREAT | O_RDWR | O_TRUNC, 0777)) == -1) {
    perror("Failed to create file");
    exit(-1);
  }

  write(fd, pid, strlen(pid));
  close(fd);
}

void stop() {
  if (unlink(".monitor_pid") == -1) {
    perror("Failed to delete .monitor_pid");
  } else {
    fprintf(stdout, "Monitor_pid ended successfully.\n");
  }
}

int main(int argc, char **argv) {
  struct sigaction sigint, sigusr1;

  sigemptyset(&sigint.sa_mask);
  sigint.sa_handler = handle_sigint;
  sigint.sa_flags = 0;
  sigaction(SIGINT, &sigint, NULL);

  sigemptyset(&sigusr1.sa_mask);
  sigusr1.sa_handler = handle_sigurs1;
  sigusr1.sa_flags = 0;
  sigaction(SIGUSR1, &sigusr1, NULL);

  start();

  while (keep_alive) {
    pause();
  }

  stop();

  return 0;
}
