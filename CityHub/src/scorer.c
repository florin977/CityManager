#include "../../CityManager/include/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

typedef struct InspectorScore {
    char username[30];
    int score;
    struct InspectorScore *next;
} InspectorScore;

int validate_district_name(const char *name) {
    if (!name || *name == '\0') return 0;
    for (int i = 0; name[i]; i++) {
        if (!isalnum(name[i])) return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <district_name>\n", argv[0]);
        exit(1);
    }

    if (!validate_district_name(argv[1])) {
        fprintf(stderr, "Scorer: Invalid district name format.\n");
        exit(1);
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", argv[1]);
    int fd = open(path, O_RDONLY);

    if (fd == -1) {
        snprintf(path, sizeof(path), "CityManager/%s/reports.dat", argv[1]);
        fd = open(path, O_RDONLY);
        if (fd == -1) {
            fprintf(stderr, "Scorer: Could not open reports for district %s\n", argv[1]);
            exit(1);
        }
    }

    InspectorScore *head = NULL;
    REPORT_DATA report;

    while (read(fd, &report, sizeof(REPORT_DATA)) == sizeof(REPORT_DATA)) {
        InspectorScore *curr = head;
        while (curr != NULL) {
            if (strncmp(curr->username, report.username, 30) == 0) {
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
            
            for (int i = 0; i < 29; i++) {
                new_score->username[i] = report.username[i];
                if (report.username[i] == '\0') break;
            }
            new_score->username[29] = '\0';
            
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
