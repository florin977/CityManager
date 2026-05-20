#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t keep_alive = 1;

void start_monitor() {
    pid_t hub_mon_pid = fork();

    if (hub_mon_pid == 0) { // Child process of hub_monitor
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Pipe failed");
            exit(-1);
        }

        pid_t monitor_pid = fork();
        if (monitor_pid == 0) { // In child process of monitor_reports
            // Close read end
            close(pipefd[0]);

            // Redirect stdout to the pipe's write end
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            execlp("./MonitorReports/monitor_reports", "./MonitorReports/monitor_reports", NULL);

        } else if (monitor_pid > 0) { // Back inside hub_monitor
            // Close write end
            close(pipefd[1]);

            char length_buffer[5] = {0};

            while (read(pipefd[0], length_buffer, 4) == 4) {
                int msg_length = atoi(length_buffer);

                if (msg_length > 0) {
                    char *msg = malloc((msg_length + 1) * sizeof(char));
                    int chars_read = 0;

                    while (chars_read < msg_length) {
                        int r = read(pipefd[0], msg + chars_read, msg_length - chars_read);
                        if (r <= 0) {
                            break;
                        }
                        chars_read += r;
                    }

                    msg[chars_read] = '\0';
                    printf("MONITOR MESSAGE: %s\n", msg);
                    free(msg);
                }
            }
            close(pipefd[0]);
            waitpid(monitor_pid, NULL, 0);
            exit(0);
        } else if (hub_mon_pid < 0) {
            perror("Failed to fork hub_monitor");
            exit(-1);
        }
    }
}

void calculate_scores(int argc, char **argv) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("Pipe failed");
        exit(-1);
    }

    // Close read end
    close(pipefd[0]);

    // Redirect stdout to the pipe's write end
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);

    for (int i = 1; i < argc; i++) {
        pid_t score_pid = fork();
        if (score_pid == 0) { // In child process of scorer
            execlp("./CityManager/scorer.sh", "./CityManager/scorer.sh", argv[i], NULL);
            perror("Error");
            char buffer[2056];

            int r = 0;
            while ((r = read(pipefd[0], buffer, 2056)) > 0) {
                write(STDOUT_FILENO, buffer, r);
            }

            exit(-1);
        } else if (score_pid < 0) {
            close(pipefd[0]);
            perror("Failed to fork scorer");
            exit(-1);
        }
    }
}

int main(int argc, char **argv) {
    calculate_scores(argc, argv);

    while (keep_alive) {
    }

    return 0;
}
