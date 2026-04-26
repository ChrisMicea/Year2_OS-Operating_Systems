#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include "../inc/test.h"

void printUsage() {
    printf("Usage:\n========= Mandatory =========\ncity_manager\t--role + <inspector/manager>\t"
           "--user + <username>\n"
           "========= add (district or report) =========\n"
           "--add + [--type district OR -d] + <district_id> [ + report_filename"
           ", if the usage mode is not type district or -d]\n"
           "========= list =========\n"
           "--list + <district_id>\n"
           "========= view =========\n"
           "--view + <district_id> + <report_id>\n"
           "========= remove_report =========\n"
           "--remove_report + <district_id> + <report_id>\n"
           "========= update_threshold =========\n"
           "--update_threshold <district_id> <value>\n"
           "========= filter =========\n"
           "--filter <district_id> <condition>\n"
        );
}

void permission_bits_to_symbolic(mode_t mode, char* out)
{
    // out must be at least 10 bytes: 9 chars + '\0'
    out[0] = (mode & S_IRUSR) ? 'r' : '-';
    out[1] = (mode & S_IWUSR) ? 'w' : '-';
    out[2] = (mode & S_IXUSR) ? 'x' : '-';
    out[3] = (mode & S_IRGRP) ? 'r' : '-';
    out[4] = (mode & S_IWGRP) ? 'w' : '-';
    out[5] = (mode & S_IXGRP) ? 'x' : '-';
    out[6] = (mode & S_IROTH) ? 'r' : '-';
    out[7] = (mode & S_IWOTH) ? 'w' : '-';
    out[8] = (mode & S_IXOTH) ? 'x' : '-';
    out[9] = '\0';
}

int handle_user_commands(int argc, char* argv[]);
int set_role(char* roleStr);
int set_user(char* userStr);
int create_district(char* districtID);
int write_report(char* reportFilepath);
int get_next_report_id(int fd);
int list_reports(char* districtID);

// for adding reports - does a cmd-line interface that takes from the user the required x, y and other data
int add_report(char* districtID);

// REFACTOR TO TAKE INTO ACCOUNT THE PERMISSION BITS OF EACH FILE
int role = -1;
char user[NAME_LEN];

int add(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    // for setting the permission bits correctly - make sure that beyond the bits set explicitly - like 0750,
    // no extra bits are modified - like the default umask = ~(0022) which is applied with &
    umask(0); 

    if (argc < 6) {
        fprintf(stderr, "Too few arguments");
        printUsage();
        return ERROR_USER_WRONG_COMMAND;
    }

    if (strcmp(argv[1], "--role")) {
        fprintf(stderr, "Invalide usage, first argument needs to be the role");
        printUsage();
        return ERROR_USER_WRONG_COMMAND;
    }

    // handle role
    if (set_role(argv[2])) {
        fprintf(stderr, "Setting role failed");
        return ERROR_HANDLE_USER_COMMAND;
    }

    if (strcmp(argv[3], "--user")) {
        fprintf(stderr, "Invalide usage, second argument needs to be the user");
        printUsage();
        return ERROR_USER_WRONG_COMMAND;
    }

    // handle_user
    if (set_user(argv[4])) {
        fprintf(stderr, "Username too long or other problem in setting the user");
        return ERROR_USER_WRONG_COMMAND;
    }

    // actual usage
    // send argc - 5 and argv + 5 as parameters because the previous 
    // cmd-line arguments have been handled
    handle_user_commands(argc - 5, argv + 5); 
}

int set_role(char* roleStr) 
{
    if (strcmp(roleStr, "manager") == 0) {
        role = MANAGER;
        return OK;
    }
    else if (strcmp(roleStr, "inspector") == 0) {
        role = INSPECTOR;
        return OK;
    }

    return ERROR_HANDLE_USER_COMMAND;
}

int set_user(char* userStr)
{
    if (strlen(userStr) > NAME_LEN) {
        return ERROR_HANDLE_USER_COMMAND;
    }

    strcpy(user, userStr);

    return OK;
}

int handle_user_commands(int argc, char* argv[])
{
    // return 0;
    int retVal;

    if (strcmp(argv[0], "--add") == 0) {
        retVal = add(argc, argv);
    }
    else if (strcmp(argv[0], "--list") == 0) {
        list_reports(argv[1]);
    }
    else if (strcmp(argv[0], "--view") == 0) {
        ;
    }
    else if (strcmp(argv[0], "--remove_report") == 0) {
        ;
    }
    else if (strcmp(argv[0], "--update_threshold") == 0) {
        ;
    }
    else if (strcmp(argv[0], "--filter") == 0) {
        ;
    }

    return retVal;
}

/*
 * wrapper that correctly selects one of the following operations: add a new district or a report in a disctrict
 */
int add(int argc, char* argv[]) 
{
    // either the optional flag -d or --type district wass added or wrong syntax
    if (argc > 2) {
        if (strcmp(argv[1], "-d") == 0 && argc == 3)
            create_district(argv[2]);
        else if(strcmp(argv[1], "--type") == 0 && strcmp(argv[2], "district") == 0 && argc == 4)
            create_district(argv[3]);
        else
            return ERROR_USER_ARGUMENTS_FOR_ADD;
    }
    else {
        add_report(argv[1]);
    }

    return OK;
}

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
            // flush buffer - if scanf() doesn't match then value remains in buffer)
            char c;
            while ((c = getchar() != '\n') && (c != EOF))
                ;
            report.severityLevel = 0; // set to arbitrary value for loop to continue
        }

    } while (report.severityLevel < 1 || report.severityLevel > 3); 

    printf("Description: ");
    // flush stdin first so the newline from previous scanf doesn't get consumed
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
    return count; // next ID - ID's start at 0 so we don't increment it
}

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