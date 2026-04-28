#include "../inc/report.h"

int add_report(char* districtID)
{
    DIR* directories = opendir("./");
    if (directories == NULL) {
        perror("opendir failed\n");
        return -1;
    }

    char symLinkName[256] = "active_reports-";
    strcat(symLinkName, districtID);

    struct dirent* currDir;
    // search for the report for the wanted district via symlink - inside project root
    while (currDir = readdir(directories)) {
        if (currDir->d_type != DT_LNK)
            continue;

        if (strcmp(symLinkName, currDir->d_name) != 0)
            continue;

        // reached here - means we found the required symbolic link
        char resolvedLink[256];
        ssize_t len = readlink(currDir->d_name, resolvedLink, sizeof(resolvedLink) - 1); // len gets updated to the no. of bytes placed into resolvedLink
        if (len == -1) {
            perror("readlink failed");
            closedir(directories);
            return -1;
        }
        resolvedLink[len] = '\0'; // readlink() does not null terminate !!!!!!!!
        closedir(directories);
        return write_report(resolvedLink);
    }

    printf("Symbolic link to the required reports file for the district not found!\n");
    printf("Checking if district exists: ...\n");
    directories = opendir(RELATIVE_FILEPATH);
    if (directories == NULL) {
        perror("opendir failed\n");
        return -1;
    }

    // search for the wanted districts inside the 'districts' folder - maybe the symlink is broken
    while (currDir = readdir(directories)) {
        if (strcmp(currDir->d_name, districtID) == 0) {
            printf("District found => Symbolic link does not exist.\n");
            char path[256];
            snprintf(path, sizeof(path), "%s%s/reports.dat", RELATIVE_FILEPATH, districtID);
            closedir(directories);
            return write_report(path);
        }
    }

    closedir(directories);
    printf("District '%s' not found.\n", districtID);
    return 5;
}

int write_report(char* reportFilepath)
{
    int fd = open(reportFilepath, O_WRONLY | O_APPEND, 0664); // 0664 if the file does not exist 
    if (fd == -1) {
        perror("open reports.dat");
        return -1;
    }

    chmod(reportFilepath, 0664); // as per requirements, ensure proper permissions for report file

    // initialize all bits of the struct to 0, important because we wirte to a binary file,
    // also, we are guaranteed to have a terminating \0 for hte user input strings
    report_t report = {0}; 

    // auto-increment ID
    report.reportID = get_next_report_id(fd);
    if (report.reportID == -1) {
        printf("problem with getting the file size of reports.dat / next report ID\n");
        return -1;
    }

    // inspector name comes from the global user[]
    strncpy(report.inspectorName, user, NAME_LEN - 1);

    report.timestamp = time(NULL);

    printf("X: ");
    if (scanf("%f", &report.coords.x) != 1) {
        printf("unexpected user input\n");
        return 2;
    }

    printf("Y: ");
    if (scanf("%f", &report.coords.y) != 1) {
        printf("unexpected user input\n");
        return 2;
    }

    printf("Issue category (road/lighting/flooding/other): ");
    scanf("%19s", report.issueCategory);  // 19 = ISSUE_CATEGORY_LEN - 1

    do {
        printf("Severity level (1/2/3): ");
        if (scanf("%d", &report.severityLevel) != 1) {
            // flush buffer - if scanf() doesn'time match then value remains in buffer)
            char c;
            while ((c = getchar() != '\n') && (c != EOF))
                ;
            report.severityLevel = 0; // set to arbitrary value for loop to continue
        }

    } while (report.severityLevel < 1 || report.severityLevel > 3); 

    printf("Description: ");
    // flush stdin first so the newline from previous scanf doesn'time get consumed
    int c; 
    while ((c = getchar()) != '\n' && c != EOF)
        ;

    fgets(report.description, DESCRIPTION_LEN, stdin);
    report.description[strcspn(report.description, "\n")] = '\0'; // strip trailing \n fgets leaves in

    // write the struct as raw bytes
    if (write(fd, &report, sizeof(report_t)) == -1) {
        perror("write failed");
        close(fd);
        return -1;
    }

    close(fd);
    printf("Report #%d added successfully.\n", report.reportID);

    return OK;
}

int get_next_report_id(int fd) {
    off_t size = lseek(fd, 0, SEEK_END); // jump to end, get file size

    if (size == -1) 
        return -1;
    
    int count = size / sizeof(report_t); // number of records already in file
    return count; // next ID - ID's start at 0 so we don'time increment it
}

int list_reports(char* districtID)
{
    char path[256];
    snprintf(path, sizeof(path), "%s%s/reports.dat", RELATIVE_FILEPATH, districtID);

    struct stat reportStats;
    if (stat(path, &reportStats) == -1) {
        perror("stat failed - does the district exist?");
        return -1;
    }

    // convert permission bits to symbolic form
    char perms[10];
    permission_bits_to_symbolic(reportStats.st_mode, perms);

    // format modification time into readable string
    char timebuf[64];
    struct tm* tm_info = localtime(&reportStats.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

    // print file info header
    printf("=== reports.dat ===\n");
    printf("Permissions: %s\n", perms);
    printf("Size: %lld bytes\n", (long long)reportStats.st_size);
    printf("Last modified: %s\n", timebuf);
    printf("Records: %lld\n", (long long)(reportStats.st_size / sizeof(report_t))); // no. of records

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open reports.dat");
        return -1;
    }

    report_t report;
    int count = 0;
    while (read(fd, &report, sizeof(report_t)) == sizeof(report_t)) { // while full reports being read
        char timeStamp[64];
        struct tm* time = localtime(&report.timestamp);
        strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d %H:%M:%S", time);

        printf("ID: %d | Inspector: %s | Category: %s | Severity: %d | Time: %s\n",
            report.reportID,
            report.inspectorName,
            report.issueCategory,
            report.severityLevel,
            timeStamp);
        count++;
    }

    if (count == 0)
        printf("No reports found in district '%s'.\n", districtID);

    close(fd);
    return OK;
}

int view_specific_report(char* districtID, char* reportID) 
{
    // convert reportID string to int
    int id = atoi(reportID);
    if (id < 0) {
        printf("Invalid report ID: '%s'\n", reportID);
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s%s/reports.dat", RELATIVE_FILEPATH, districtID);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open reports.dat");
        return -1;
    }

    // jump directly to the record: IDs are 0-based
    off_t offset = (off_t)(id) * sizeof(report_t);
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek failed");
        close(fd);
        return -1;
    }

    report_t report;
    ssize_t bytes_read = read(fd, &report, sizeof(report_t));
    close(fd);

    if (bytes_read != sizeof(report_t)) {
        printf("Report #%d not found in district '%s', error in jumping with offset given by reportID.\n", id, districtID);
        return -1;
    }

    // format timestamp
    char timeStamp[64];
    struct tm* time = localtime(&report.timestamp);
    strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d %H:%M:%S", time);

    // print full details
    printf("========== Report #%d ==========\n", report.reportID);
    printf("Inspector: %s\n",   report.inspectorName);
    printf("Timestamp: %s\n",   timeStamp);
    printf("Latitude: %f\n", report.coords.x);
    printf("Longitude: %f\n", report.coords.y);
    printf("Category: %s\n",   report.issueCategory);
    printf("Severity: %d - ", report.severityLevel);
    if (report.severityLevel == 1)
        printf("minor\n");
    else if (report.severityLevel == 2)
        printf("moderate\n");
    else
        printf("critical\n");
    printf("Description: %s\n",   report.description);
    printf("================================\n");

    return OK;
}

int remove_report(char* districtID, char* reportID)
{
    // manager only
    if (role != MANAGER) {
        printf("Permission denied: remove_report is for managers only.\n");
        return -1;
    }

    int id = atoi(reportID);
    if (id < 0) {
        printf("Invalid report ID: '%s'\n", reportID);
        return -1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s%s/reports.dat", RELATIVE_FILEPATH, districtID);

    int fd = open(path, O_RDWR);
    if (fd == -1) {
        perror("open reports.dat");
        return -1;
    }

    off_t filesize = lseek(fd, 0, SEEK_END);
    if (filesize == -1) {
        perror("lseek failed");
        close(fd);
        return -1;
    }

    int total_records = filesize / sizeof(report_t);

    // find the record with the matching reportID
    int target_pos = -1;
    report_t report;
    for (int i = 0; i < total_records; i++) {
        lseek(fd, (off_t)i * sizeof(report_t), SEEK_SET);
        if (read(fd, &report, sizeof(report_t)) != sizeof(report_t)) {
            perror("read failed");
            close(fd);
            return -1;
        }
        if (report.reportID == id) {
            target_pos = i;
            break;
        }
    }

    if (target_pos == -1) {
        printf("Report #%d not found in district '%s'.\n", id, districtID);
        close(fd);
        return -1;
    }

    // shift every record after target_pos one position earlier
    for (int i = target_pos + 1; i < total_records; i++) {
        // read record at position i
        lseek(fd, (off_t)i * sizeof(report_t), SEEK_SET);
        if (read(fd, &report, sizeof(report_t)) != sizeof(report_t)) {
            perror("read failed during shift");
            close(fd);
            return -1;
        }

        // write it one position earlier
        lseek(fd, (off_t)(i - 1) * sizeof(report_t), SEEK_SET);
        if (write(fd, &report, sizeof(report_t)) != sizeof(report_t)) {
            perror("write failed during shift");
            close(fd);
            return -1;
        }
    }

    // truncate the file by exactly one record
    if (ftruncate(fd, filesize - sizeof(report_t)) == -1) {
        perror("ftruncate failed");
        close(fd);
        return -1;
    }

    close(fd);
    printf("Report #%d removed successfully.\n", id);
    return OK;
}

int parse_condition(const char *input, char *field, char *op, char *value)
{
    if (!input || !field || !op || !value) {
        fprintf(stderr, "parse_condition: NULL argument\n");
        return -1;
    }

    printf("input is %s\n", input);

    /* Find the first colon — separates field from operator */
    const char *first_colon = strchr(input, ':');
    if (!first_colon) {
        fprintf(stderr, "parse_condition: missing first ':' in \"%s\"\n", input);
        return -1;
    }

    /* Find the second colon — separates operator from value */
    const char *second_colon = strchr(first_colon + 1, ':');
    if (!second_colon) {
        fprintf(stderr, "parse_condition: missing second ':' in \"%s\"\n", input);
        return -1;
    }

    /* Extract field */
    size_t field_len = first_colon - input;
    if (field_len == 0) {
        fprintf(stderr, "parse_condition: empty field in \"%s\"\n", input);
        return -1;
    }
    strncpy(field, input, field_len);
    field[field_len] = '\0';

    /* Extract operator */
    size_t op_len = second_colon - (first_colon + 1);
    if (op_len == 0 || op_len > 2) {   /* longest valid op is ">=", 2 chars */
        fprintf(stderr, "parse_condition: bad operator length in \"%s\"\n", input);
        return -1;
    }
    strncpy(op, first_colon + 1, op_len);
    op[op_len] = '\0';

    /* Validate operator is one of the recognised tokens */
    if (strcmp(op, "==") != 0 && strcmp(op, "!=") != 0 &&
        strcmp(op, ">")  != 0 && strcmp(op, "<")  != 0 &&
        strcmp(op, ">=") != 0 && strcmp(op, "<=") != 0) 
        {
        fprintf(stderr, "parse_condition: unrecognised operator \"%s\"\n", op);
        return -1;
    }

    /* Extract value — everything after the second colon */
    const char *val_start = second_colon + 1;
    if (*val_start == '\0') {
        fprintf(stderr, "parse_condition: empty value in \"%s\"\n", input);
        return -1;
    }
    strcpy(value, val_start);   /* safe: caller owns a buffer sized to the input */

    return OK;   /* OK */
}

/* ── internal helpers ─────────────────────────────────────────────────── */

/* apply op to two longs; all six operators */
static int cmp_long(long lhs, const char *op, long rhs)
{
    if (strcmp(op, "==") == 0) return lhs == rhs;
    if (strcmp(op, "!=") == 0) return lhs != rhs;
    if (strcmp(op, "<")  == 0) return lhs <  rhs;
    if (strcmp(op, ">")  == 0) return lhs >  rhs;
    if (strcmp(op, "<=") == 0) return lhs <= rhs;
    if (strcmp(op, ">=") == 0) return lhs >= rhs;
    return 0;
}

/* apply op to two doubles */
static int cmp_double(double lhs, const char *op, double rhs)
{
    if (strcmp(op, "==") == 0) return lhs == rhs;   /* exact float compare —
                                                        fine for filter use */
    if (strcmp(op, "!=") == 0) return lhs != rhs;
    if (strcmp(op, "<")  == 0) return lhs <  rhs;
    if (strcmp(op, ">")  == 0) return lhs >  rhs;
    if (strcmp(op, "<=") == 0) return lhs <= rhs;
    if (strcmp(op, ">=") == 0) return lhs >= rhs;
    return 0;
}

/* apply op to two strings; only == and != are permitted */
static int cmp_string(const char *lhs, const char *op, const char *rhs)
{
    if (strcmp(op, "==") == 0) return strcmp(lhs, rhs) == 0;
    if (strcmp(op, "!=") == 0) return strcmp(lhs, rhs) != 0;

    fprintf(stderr,
            "match_condition: operator \"%s\" is not valid for string fields\n",
            op);
    return 0;
}

/* ── public function ──────────────────────────────────────────────────── */
int match_condition(const report_t *report, const char *field, const char *op, const char *value)
{
    if (!report || !field || !op || !value) {
        fprintf(stderr, "match_condition: NULL argument\n");
        return 0;
    }

    /* ── numeric fields ─────────────────────────────────────────────── */

    if (strcmp(field, "id") == 0) {
        long rhs = atol(value);
        return cmp_long((long)report->reportID, op, rhs);
    }

    if (strcmp(field, "severity") == 0) {
        long rhs = atol(value);
        if (rhs < 1 || rhs > 3) {
            fprintf(stderr,
                    "match_condition: severity value must be 1, 2 or 3 (got \"%s\")\n",
                    value);
            return 0;
        }
        return cmp_long((long)report->severityLevel, op, rhs);
    }

    if (strcmp(field, "timestamp") == 0) {
        long rhs = atol(value);
        return cmp_long((long)report->timestamp, op, rhs);
    }

    if (strcmp(field, "x") == 0 || strcmp(field, "lat") == 0) {
        double rhs = atof(value);
        return cmp_double((double)report->coords.x, op, rhs);
    }

    if (strcmp(field, "y") == 0 || strcmp(field, "lon") == 0) {
        double rhs = atof(value);
        return cmp_double((double)report->coords.y, op, rhs);
    }

    /* ── string fields ──────────────────────────────────────────────── */

    if (strcmp(field, "inspector") == 0)
        return cmp_string(report->inspectorName, op, value);

    if (strcmp(field, "category") == 0)
        return cmp_string(report->issueCategory, op, value);

    if (strcmp(field, "description") == 0)
        return cmp_string(report->description, op, value);

    /* ── unknown field ──────────────────────────────────────────────── */

    fprintf(stderr, "match_condition: unknown field \"%s\"\n", field);
    return 0;
}

int filter_reports(char *districtID, char **conditions, int num_conditions)
{
    // parse all conditions up front — fail fast before opening any file
    char fields[num_conditions][32];
    char ops[num_conditions][4];
    char values[num_conditions][DESCRIPTION_LEN];

    for (int i = 0; i < num_conditions; i++) {
        if (parse_condition(conditions[i], fields[i], ops[i], values[i]) != 0) {
            fprintf(stderr, "Invalid condition: \"%s\"\n", conditions[i]);
            fprintf(stderr, "Expected format: field:op:value  e.g. severity:>:2\n");
            return -1;
        }
    }

    char path[256];
    snprintf(path, sizeof(path), "%s%s/reports.dat", RELATIVE_FILEPATH, districtID);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open reports.dat");
        return -1;
    }

    report_t report;
    int count = 0; // for print at the end if no records matching the condition are met
    while (read(fd, &report, sizeof(report_t)) == sizeof(report_t)) {
        int match = 1;
        for (int i = 0; i < num_conditions; i++) { // record should satisfy all conditions
            if (!match_condition(&report, fields[i], ops[i], values[i])) {
                match = 0;
                break;
            }
        }

        if (match) {
            char timeStamp[64];
            struct tm* time = localtime(&report.timestamp);
            strftime(timeStamp, sizeof(timeStamp), "%Y-%m-%d %H:%M:%S", time);

            printf("ID: %d | Inspector: %s | Category: %s | Severity: %d | Time: %s\n",
                report.reportID,
                report.inspectorName,
                report.issueCategory,
                report.severityLevel,
                timeStamp);
            count++;
        }
    }

    close(fd);

    if (count == 0)
        printf("No reports match all conditions in district '%s'.\n", districtID);
    else
        printf("%d report(s) matched\n", count);

    return OK;
}
