#include "../inc/utils.h"


void print_usage() {
    printf("Usage:\n========= Mandatory =========\ncity_manager\time--role + <inspector/manager>\time"
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