#include "../inc/commons.h"
#include "../inc/utils.h"
#include "../inc/district.h"
#include "../inc/report.h"

role_t role = -1;
char user[NAME_LEN];

int handle_user_commands(int argc, char* argv[]);
int set_role(char* roleStr);
int set_user(char* userStr);
int add(int argc, char* argv[]);

int main(int argc, char* argv[]) {
    // for setting the permission bits correctly - make sure that beyond the bits set explicitly - like 0750,
    // no extra bits are modified - like the default umask = ~(0022) which is applied with &
    umask(0); 

    if (argc < 6) {
        fprintf(stderr, "Too few arguments\n");
        print_usage();
        return ERROR_USER_WRONG_COMMAND;
    }

    if (strcmp(argv[1], "--role")) {
        fprintf(stderr, "Invalide usage, first argument needs to be the role\n");
        print_usage();
        return ERROR_USER_WRONG_COMMAND;
    }

    // handle role
    if (set_role(argv[2])) {
        fprintf(stderr, "Setting role failed\n");
        return ERROR_HANDLE_USER_COMMAND;
    }

    if (strcmp(argv[3], "--user")) {
        fprintf(stderr, "Invalide usage, second argument needs to be the user\n");
        print_usage();
        return ERROR_USER_WRONG_COMMAND;
    }

    // handle_user
    if (set_user(argv[4])) {
        fprintf(stderr, "Username too long or other problem in setting the user\n");
        return ERROR_USER_WRONG_COMMAND;
    }

    // actual usage
    // send argc - 5 and argv + 5 as parameters because the previous 
    // cmd-line arguments have been handled
    int retVal = handle_user_commands(argc - 5, argv + 5);
    
    if (retVal != 0) {
        printf("Error applying user command\n");
    }
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
    int retVal = -1;

    if (strcmp(argv[0], "--add") == 0) {
        retVal = add(argc, argv);
    }
    else if (strcmp(argv[0], "--list") == 0) {
        if (argc != 2) {
            printf("Incorrect usage - not enough or too many arguments for 'list'\n");
            print_usage();
        }
        else {
            retVal = list_reports(argv[1]);
        }
    }
    else if (strcmp(argv[0], "--view") == 0) {
        if (argc != 3) {
            printf("Incorrect usage - not enough or too many arguments for 'view'\n");
            print_usage();
        }
        else {
            retVal = view_specific_report(argv[1], argv[2]);
        }
    }
    else if (strcmp(argv[0], "--remove_report") == 0) {
        if (argc != 3) {
            printf("Incorrect usage - not enough or too many arguments for 'remove report'\n");
            print_usage();
        }
        else {
            retVal = remove_report(argv[1], argv[2]);
        }
    }
    else if (strcmp(argv[0], "--update_threshold") == 0) {
        if (argc != 3) {
            printf("Incorrect usage - not enough or too many arguments for 'update threshold'\n");
            print_usage();
        }
        else {
            retVal = update_threshold(argv[1], argv[2]);
        }
    }
    else if (strcmp(argv[0], "--filter") == 0) {
        if (argc < 3) {
            printf("Incorrect usage - not enough arguments for 'filter'\n");
            print_usage();
        }
        else {
            // argv[1] = districtID, argv[2..] = one condition per slot
            int nConditions = argc - 2;
            retVal = filter_reports(argv[1], argv + 2, nConditions);
        }
    }
    else if (strcmp(argv[0], "--remove_district") == 0) {
        if (argc != 2) {
            printf("Incorrect usage - not enough or too many arguments for 'remove_district'\n");
            print_usage();
        }
        else {
            retVal = remove_district(argv[1]);
        }
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