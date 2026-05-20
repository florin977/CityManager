#include "../include/types.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid argument count\n");
        exit(-1);
    }

    char *district = argv[1];
    char inspectors[10][256];

    // List all the reports in this district and save them in a buffer



    COMMAND aux;
    aux.argc = 2;
    aux.role = MANAGER;
    aux.type = FILTER;
    strcpy(aux.username, "Hub Monitor");

    return 0;
}
