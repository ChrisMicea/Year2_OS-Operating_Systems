#include "../inc/commons.h"

char hidden_filepath[256];

int create_hidden_monitor_file();
void handle_sigint(int sig);
void handle_sigusr1(int sig);
void handle_sigusr2(int sig);

int main() {
    // SIGINT handler
    struct sigaction sigint_handler;
    memset(&sigint_handler, 0x0, sizeof(struct sigaction));
    sigint_handler.sa_handler = handle_sigint;
    if (sigaction(SIGINT, &sigint_handler, NULL) < 0) {
        perror("Processing SIGINT sigaction");
        exit(1);
    }

    // SIGUSR1 handler
    struct sigaction sigusr1_handler;
    memset(&sigusr1_handler, 0x0, sizeof(struct sigaction));
    sigusr1_handler.sa_handler = handle_sigusr1;
    if (sigaction(SIGUSR1, &sigusr1_handler, NULL) < 0) {
        perror("Processing SIGUSR1 sigaction");
        exit(1);
    }

    // SIGUSR2 handler
    struct sigaction sigusr2_handler;
    memset(&sigusr2_handler, 0x0, sizeof(struct sigaction));
    sigusr2_handler.sa_handler = handle_sigusr2;
    if (sigaction(SIGUSR2, &sigusr2_handler, NULL) < 0) {
        perror("Processing SIGUSR2 sigaction");
        exit(1);
    }

    create_hidden_monitor_file();

    while (1)
        pause(); // pause instead of ';' to sleep the process until it receives a signal (';' uses CPU)
    
    return 0;
}

int create_hidden_monitor_file()
{
    char hidden_filename[] = ".monitor_pid";
    int fd;

    // Safety check before building the path
    if (strlen(RELATIVE_FILEPATH) + strlen(hidden_filename) + 5 > sizeof(hidden_filepath)) { // 5 for safety net (no precise maths for taking into account '\0')
        printf("district ID too long\n");
        return -1;
    }

    strcat(hidden_filepath, RELATIVE_FILEPATH);
    strcat(hidden_filepath, hidden_filename);
    // 0666 means rw for everyone (could maybe trim it down to someting like rw for owner, r for everyone else - 0644)
    if ((fd = creat(hidden_filepath, 0666)) == -1) { // or open(path, O_CREAT | O_WRONLY | O_EXCL);
        perror("creat failed");
        return -1;
    }

    pid_t pid = getpid();
    char buf[16];
    int len = snprintf(buf, sizeof(buf), "%d\n", pid);

    if (write(fd, buf, len) == -1) {
        perror("write failed");
        close(fd);
        return -1;
    }

    close(fd);
    return OK;
}

void handle_sigint(int sig)
{
    (void) sig; // statement with no effect, suppresses "unused parameter" warning

    char msg[] = "\nSIGINT received - ending the monitor process\n";

    // this will fail if interrupt is sent before create_hidden_monitor_file() is called
    if (unlink(hidden_filepath) < 0) {
        perror("Deleting hidden .monitor_pid file");
    } 

    // same as strlen(msg) plus the terminating '\0' ||| can use sizeof(msg) - 1 to be performative
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    _exit(0); // apparently, better and safer than exit(), it doesn't flush stdio buffers
}

// SIGUSR1 = report modification
void handle_sigusr1(int sig)
{
    (void) sig; // suppress warning, same as the same line of code in handle_sigint()

    char msg[] = "\nSIGUSR1 received - report modified (added or deleted)\n";

    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

// SIGUSR2 = district modification
void handle_sigusr2(int sig)
{
    (void) sig; // suppress warning, same as the same line of code in handle_sigint()

    char msg[] = "\nSIGUSR2 received - district modified (added or deleted)\n";

    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}