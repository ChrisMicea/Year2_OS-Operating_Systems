#ifndef COMMONS_H

#define COMMONS_H

// header / library for defines, common datatypes and data structures (and all of the weir libraries used for system calls)
// as well as standard libraries that are needed everywhere

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#define NAME_LEN 20
#define ISSUE_CATEGORY_LEN 20
#define DESCRIPTION_LEN 200
#define RELATIVE_FILEPATH "./districts/" 

typedef struct {
    int reportID;
    char inspectorName[NAME_LEN];
    struct {
        float x, y;
    } coords;
    char issueCategory[ISSUE_CATEGORY_LEN];
    int severityLevel; // 1 = minor, 2 = moderate, 3 = critical
    time_t timestamp;
    char description[DESCRIPTION_LEN];
} report_t;

typedef enum {
    MANAGER, INSPECTOR
} role_t;

typedef enum {
    OK, 
    ERROR_USER_WRONG_COMMAND,
    ERROR_HANDLE_USER_COMMAND,
    ERROR_USER_ARGUMENTS_FOR_ADD,
    ERROR
} returnType_t;

extern role_t role;
extern char user[NAME_LEN];

#endif // COMMONS_H