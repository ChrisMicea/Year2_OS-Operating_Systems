#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
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

int handle_user_commands(int argc, char* argv[]);
int set_role(char* roleStr);
int set_user(char* userStr);
int create_district(char* districtID);

// for adding reports - does a cmd-line interface that takes from the user the required x, y and other data
int add_report(char* districtID);

// REFACTOR TO TAKE INTO ACCOUNT THE PERMISSION BITS OF EACH FILE
int role = -1;
char user[NAME_LEN];

int add(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    if (argc < 6) {
        fprintf(stderr, "Too few arguments");
        printUsage();
        return USER_WRONG_COMMAND;
    }

    if (strcmp(argv[1], "--role")) {
        fprintf(stderr, "Invalide usage, first argument needs to be the role");
        printUsage();
        return USER_WRONG_COMMAND;
    }

    // handle role
    if (set_role(argv[2])) {
        fprintf(stderr, "Setting role failed");
        return ERROR_HANDLE_USER_COMMAND;
    }

    if (strcmp(argv[3], "--user")) {
        fprintf(stderr, "Invalide usage, second argument needs to be the user");
        printUsage();
        return USER_WRONG_COMMAND;
    }

    // handle_user
    if (set_user(argv[4])) {
        fprintf(stderr, "Username too long or other problem in setting the user");
        return USER_WRONG_COMMAND;
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
        ;
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
    // printf("%d\n", argc);
    // for (int i = 0; i < argc; i++)
    //     printf("%s\n", argv[i]);

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

int create_district(char* districtID)
{
    return OK;
}