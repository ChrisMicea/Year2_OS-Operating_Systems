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

void print_usage_hub() {
    printf("Usage:\n"
            "start_monitor - launch the monitor background process\n"
            "calculate_scores <list_of_districts> - calculate the inspector rating for each district given as parameter\n"
            "help - show this message\n"
            "exit / quit - quit city_hub\n"
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

// signal as a parameter because we can either send SIGUSR1 for reports or SIGUSR2 for districts
// return monitor_pid on success or a specific error code on failure
int notify_monitor(int signal)
{
    char pid_path[256];
    snprintf(pid_path, sizeof(pid_path), "%s.monitor_pid", RELATIVE_FILEPATH);

    int fd = open(pid_path, O_RDONLY);
    if (fd == -1)
        return -1; // no PID file — monitor not running

    // read PID from file (.monitor_pid)
    char buf[16];
    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (len <= 0)
        return -2; // empty or unreadable file
    buf[len] = '\0';

    pid_t monitor_pid = (pid_t)(atoi(buf));
    if (monitor_pid <= 0)
        return -3; // invalid PID

    if (kill(monitor_pid, signal) == -1)
        return -4; // couldn't send process (or permission denied)

    return monitor_pid; // return PID so caller can log it
}

int log_event(char* districtID, char* message)
{
    char path[256];
    snprintf(path, sizeof(path), "%s%s/logged_district.txt", RELATIVE_FILEPATH, districtID);

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("open logged_district.txt");
        return -1;
    }

    // build timestamp
    char timebuf[32];
    time_t now = time(NULL); // returns current time
    struct tm* tm_info = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

    // write the log
    char entry[512];
    int len = snprintf(entry, sizeof(entry), "[%s] %s\n", timebuf, message);
    if (write(fd, entry, len) == -1) {
        perror("write to logged_district.txt");
        close(fd);
        return -1;
    }

    close(fd);
    return OK;
}