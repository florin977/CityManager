#include "../../CityManager/include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct InspectorScore {
    char username[30];
    int score;
    struct InspectorScore *next;
} InspectorScore;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <district_name>\n", argv[0]);
        return 1;
    }

    char path[512];
    // Prioritize district in current directory (where city_manager usually creates them)
    snprintf(path, sizeof(path), "%s/reports.dat", argv[1]);
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        // Fallback to CityManager directory
        snprintf(path, sizeof(path), "CityManager/%s/reports.dat", argv[1]);
        fd = open(path, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Scorer: Could not open reports for district %s\n", argv[1]);
            return 1;
        }
    }

    InspectorScore *head = NULL;
    REPORT_DATA report;

    while (read(fd, &report, sizeof(REPORT_DATA)) == sizeof(REPORT_DATA)) {
        InspectorScore *curr = head;
        while (curr != NULL) {
            if (strcmp(curr->username, report.username) == 0) {
                curr->score += report.severity_level;
                break;
            }
            curr = curr->next;
        }
        if (curr == NULL) {
            InspectorScore *new_score = malloc(sizeof(InspectorScore));
            if (!new_score) {
                perror("malloc");
                exit(1);
            }
            strncpy(new_score->username, report.username, 30);
            new_score->score = report.severity_level;
            new_score->next = head;
            head = new_score;
        }
    }

    close(fd);

    printf("District %s:\n", argv[1]);
    InspectorScore *curr = head;
    if (curr == NULL) {
        printf("No reports found.\n");
    }
    while (curr != NULL) {
        printf("Inspector %s: %d\n", curr->username, curr->score);
        InspectorScore *tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    return 0;
}
