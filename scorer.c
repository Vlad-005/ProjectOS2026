#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_INSPECTORS 256
#define MAX_NAME 64
#define MAX_DESC 128

typedef struct {
    int id;
    char inspector[MAX_NAME];
    float lat, lon;
    char category[MAX_NAME];
    int severity;
    time_t timestamp;
    char description[MAX_DESC];
} Report;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: scorer <district>\n");
        return 1;
    }

    const char *district = argv[1];
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("No reports found for district %s or cannot open %s\n", district, path);
        return 0;
    }

    struct {
        char inspector[MAX_NAME];
        int score;
    } inspectors[MAX_INSPECTORS];
    int inspector_count = 0;

    Report r;
    while (read(fd, &r, sizeof(r)) == sizeof(r)) {
        int found = 0;
        for (int i = 0; i < inspector_count; ++i) {
            if (strcmp(inspectors[i].inspector, r.inspector) == 0) {
                inspectors[i].score += r.severity;
                found = 1;
                break;
            }
        }
        if (!found && inspector_count < MAX_INSPECTORS) {
            strncpy(inspectors[inspector_count].inspector, r.inspector, MAX_NAME - 1);
            inspectors[inspector_count].inspector[MAX_NAME - 1] = '\0';
            inspectors[inspector_count].score = r.severity;
            inspector_count++;
        }
    }

    close(fd);

    if (inspector_count == 0) {
        printf("No inspectors with workload in district %s.\n", district);
        return 0;
    }

    for (int i = 0; i < inspector_count; ++i) {
        printf("Inspector: %s, Workload Score: %d\n", inspectors[i].inspector, inspectors[i].score);
    }

    return 0;
}
