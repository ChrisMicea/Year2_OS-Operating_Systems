#include "../inc/commons.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: scorer <districtID>\n");
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s%s/reports.dat", RELATIVE_FILEPATH, argv[1]);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "[scorer] Cannot open %s: %s\n", path, strerror(errno));
        return 1;
    }

    // accumulate scores per inspector name
    char names[MAX_INSPECTORS][NAME_LEN];
    int scores[MAX_INSPECTORS];
    int count = 0;

    report_t report;
    while (read(fd, &report, sizeof(report_t)) == sizeof(report_t)) {
        // find existing inspector entry
        int found = -1;
        for (int i = 0; i < count; i++) {
            if (strcmp(names[i], report.inspectorName) == 0) {
                found = i;
                break;
            }
        }

        if (found == -1) {
            // new inspector
            if (count >= MAX_INSPECTORS) {
                fprintf(stderr, "[scorer] Too many inspectors in district %s\n", argv[1]);
                break;
            }
            strncpy(names[count], report.inspectorName, NAME_LEN - 1);
            names[count][NAME_LEN - 1] = '\0';
            scores[count] = report.severityLevel;
            count++;
        } 
        else {
            scores[found] += report.severityLevel;
        }
    }

    close(fd);

    // print plain-text summary to stdout (piped back to city_hub)
    printf("District: %s\n", argv[1]);
    if (count == 0) {
        printf(" (no reports)\n");
    } 
    else {
        for (int i = 0; i < count; i++)
            printf(" Inspector %-20s workload score: %d\n", names[i], scores[i]);
    }

    return 0;
}