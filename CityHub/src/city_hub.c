#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

void start_monitor() {
    pid_t hub_mon_pid = fork();

    if (hub_mon_pid == 0) { // hub_mon
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Pipe failed");
            exit(-1);
        }

        pid_t monitor_pid = fork();
        if (monitor_pid == 0) { // monitor
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            // Run the monitor
            execlp("./MonitorReports/monitor_reports", "monitor_reports", NULL);
            perror("execlp monitor");
            exit(-1);
        } else if (monitor_pid > 0) {
            close(pipefd[1]);
            char length_buffer[5] = {0};
            while (read(pipefd[0], length_buffer, 4) == 4) {
                length_buffer[4] = '\0';
                int msg_length = atoi(length_buffer);
                if (msg_length > 0) {
                    char *msg = malloc(msg_length + 1);
                    int total_read = 0;
                    while (total_read < msg_length) {
                        int r = read(pipefd[0], msg + total_read, msg_length - total_read);
                        if (r <= 0) break;
                        total_read += r;
                    }
                    msg[total_read] = '\0';
                    printf("\n[MONITOR] %s\n> ", msg);
                    fflush(stdout);
                    free(msg);
                }
            }
            printf("\n[HUB_MON] Monitor process has ended.\n> ");
            fflush(stdout);
            close(pipefd[0]);
            waitpid(monitor_pid, NULL, 0);
            exit(0);
        } else {
            perror("fork monitor");
            exit(-1);
        }
    } else if (hub_mon_pid < 0) {
        perror("fork hub_mon");
    } else {
        printf("hub_mon started in background with PID %d\n", hub_mon_pid);
    }
}

void calculate_scores(int count, char **districts) {
    if (count == 0) {
        printf("Usage: calculate_scores <district1> <district2> ...\n");
        return;
    }

    int pipes[64][2];
    pid_t pids[64];

    for (int i = 0; i < count; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            pids[i] = -1;
            continue;
        }
        pids[i] = fork();
        if (pids[i] == 0) { // Scorer child
            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]);
            // Close other pipes from previous iterations
            for (int j = 0; j < i; j++) {
                if (pids[j] != -1) close(pipes[j][0]);
            }
            // Use ./CityHub/scorer as it's built there
            execlp("./CityHub/scorer", "scorer", districts[i], NULL);
            perror("execlp scorer");
            exit(-1);
        } else if (pids[i] > 0) {
            close(pipes[i][1]);
        } else {
            perror("fork scorer");
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
    }

    printf("--- Combined Workload Report ---\n");
    for (int i = 0; i < count; i++) {
        if (pids[i] <= 0) continue;
        char buffer[1024];
        int n;
        while ((n = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = 0;
            printf("%s", buffer);
        }
        close(pipes[i][0]);
        waitpid(pids[i], NULL, 0);
    }
    printf("--------------------------------\n");
}

int main() {
    char line[1024];
    printf("CityHub CLI started. Type 'help' for commands.\n");

    while (1) {
        printf("> ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin) == NULL) break;

        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;

        char *cmd = strtok(line, " ");
        if (cmd == NULL) continue;

        if (strcmp(cmd, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(cmd, "calculate_scores") == 0) {
            char *districts[64];
            int count = 0;
            char *arg;
            while ((arg = strtok(NULL, " ")) != NULL && count < 64) {
                districts[count++] = arg;
            }
            calculate_scores(count, districts);
        } else if (strcmp(cmd, "help") == 0) {
            printf("Commands:\n");
            printf("  start_monitor                     - Start the monitor in background\n");
            printf("  calculate_scores <d1> <d2> ...    - Calculate workload scores for districts\n");
            printf("  exit                              - Exit the CLI\n");
        } else if (strcmp(cmd, "exit") == 0) {
            break;
        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }
    return 0;
}
