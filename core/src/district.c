#include "../inc/district.h"

int create_district(char* districtID)
{
    char path[256] = "";
    int fd;

    // Safety check before building the path
    if (strlen(RELATIVE_FILEPATH) + strlen(districtID) + 5 > sizeof(path)) { // 5 for safety net (no precise maths for taking into account '\0')
        printf("district ID too long\n");
        return -1;
    }

    // Create the district directory with rwxr-x--- (750)
    strcat(path, RELATIVE_FILEPATH);
    strcat(path, districtID);
    if (mkdir(path, 0750) == -1) {
        perror("mkdir failed");
        return -1;
    }
    chmod(path, 0750);

    // Create reports.dat with rw-rw-r-- (664)
    snprintf(path, sizeof(path), "%s%s/reports.dat", RELATIVE_FILEPATH, districtID);
    fd = open(path, O_CREAT | O_WRONLY | O_EXCL, 0664);
    if (fd == -1) {
        perror("open reports.dat");
        return -1;
    }
    // chmod(path, 0664); // or fchmod(fd, 0664)
    close(fd);

    // Create district.cfg with rw-r----- (640)
    snprintf(path, sizeof(path), "%s%s/district.cfg", RELATIVE_FILEPATH, districtID);
    fd = open(path, O_CREAT | O_WRONLY | O_EXCL, 0640);
    if (fd == -1) {
        perror("open district.cfg");
        return -1;
    }
    // chmod(path, 0640); // or fchmod(fd, 0640)
    close(fd);

    // Create logged_district.txt with rw-r--r-- (644)
    snprintf(path, sizeof(path), "%s%s/logged_district.txt", RELATIVE_FILEPATH, districtID);
    fd = open(path, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (fd == -1) {
        perror("open logged_district.txt");
        return -1;
    }
    // chmod(path, 0644); // or fchmod(fd, 0644)
    close(fd);

    // Create symbolic link: active_reports-<districtID> -> <districtID>/reports.dat
    char link_name[256], target[256];
    snprintf(link_name, sizeof(link_name), "./active_reports-%s", districtID);
    snprintf(target, sizeof(target), "%s%s/reports.dat", RELATIVE_FILEPATH, districtID);
    if (symlink(target, link_name) == -1) {
        perror("symlink failed");
        return -1;
    }

    return 0;
}

int update_threshold(char* districtID, char* value)
{
    // check manager role
    if (role != MANAGER) {
        printf("Permission denied: update_threshold is for managers only.\n");
        return -1;
    }

    int threshold = atoi(value);
    if (threshold < 1 || threshold > 3) {
        printf("Invalid threshold value '%s': must be 1, 2, or 3.\n", value);
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s%s/district.cfg", RELATIVE_FILEPATH, districtID);

    // stat() first to verify permission bits
    struct stat fileStats;
    if (stat(path, &fileStats) == -1) {
        perror("stat district.cfg failed");
        return -1;
    }

    // mask out only the permission bits from fileStats.st_mode
    // fileStats.st_mode also contains file type bits, so & with 0777 to isolate permissions
    mode_t perms = fileStats.st_mode & 0777;
    if (perms != 0640) {
        char symbolic[10];
        permission_bits_to_symbolic(fileStats.st_mode, symbolic);
        printf("Security error: district.cfg has unexpected permissions %s (expected rw-r-----).\n", symbolic);
        printf("Refusing to write. Restore permissions with: chmod 640 %s\n", path);
        return -1;
    }

    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("open district.cfg");
        return -1;
    }

    char buf[3];
    int len = snprintf(buf, sizeof(buf), "%d\n", threshold);
    if (write(fd, buf, len) == -1) {
        perror("write failed");
        close(fd);
        return -1;
    }

    close(fd);
    printf("Threshold updated to %d for district '%s'.\n", threshold, districtID);
    return OK;
}
