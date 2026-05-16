#include "../inc/commons.h"

// Message types sent through the pipe (or stdout when run standalone)
// Protocol: TYPE:LENGTH:payload\n
// TYPE: ERROR | EVENT | EXIT
// LENGTH: decimal byte count of payload (not including the \n)
// payload: human-readable message

// added for milestone 3
#define MSG_ERROR "ERROR"
#define MSG_EVENT "EVENT"
#define MSG_EXIT "EXIT"


char hidden_filepath[256];

int create_hidden_monitor_file();
void handle_sigint(int sig);
void handle_sigusr1(int sig);
void handle_sigusr2(int sig);
void send_msg(const char *type, const char *payload); // added for milestone 3

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


    // added for milestone 3
    // Check if another monitor is already running BEFORE creating the pid file
    char pid_path[256];
    snprintf(pid_path, sizeof(pid_path), "%s.monitor_pid", RELATIVE_FILEPATH);
 
    int existing_fd = open(pid_path, O_RDONLY);
    if (existing_fd != -1) {
        char buf[16];
        ssize_t len = read(existing_fd, buf, sizeof(buf) - 1);
        close(existing_fd);
 
        if (len > 0) {
            buf[len] = '\0';
            // strip trailing newline
            buf[strcspn(buf, "\n")] = '\0';
            pid_t existing_pid = (pid_t)atoi(buf);
 
            // verify the process actually exists
            if (existing_pid > 0 && kill(existing_pid, 0) == 0) {
                char err_payload[64];
                snprintf(err_payload, sizeof(err_payload), "Monitor already running with PID %d", existing_pid);
                send_msg(MSG_ERROR, err_payload);
 
                // also send EXIT so hub_mon knows we are done
                send_msg(MSG_EXIT, "Monitor aborted - duplicate instance detected");
                _exit(1);
            }
        }
    }

    // modified from milestone 2 by adding error checking instead of just create_hidden_monitor_file()
    if (create_hidden_monitor_file() < 0) {
        send_msg(MSG_ERROR, "Failed to create .monitor_pid file");
        send_msg(MSG_EXIT, "Monitor aborted - could not create PID file");
        _exit(1);
    }

    send_msg(MSG_EVENT, "Monitor started successfully");

    while (1)
        pause(); // pause instead of ';' to sleep the process until it receives a signal (';' uses CPU)
    
    return 0;
}

// Format: TYPE:LENGTH:payload\n
void send_msg(const char *type, const char *payload)
{
    // build the full frame in a fixed buffer
    // max: 5 (type) + 1 + 4 (length up to 9999) + 1 + payload + 1 (\n) + \0
    char frame[512];
    int payload_len = (int)strlen(payload);
    int frame_len = snprintf(frame, sizeof(frame), "%s:%d:%s\n", type, payload_len, payload);
    if (frame_len > 0)
        write(STDOUT_FILENO, frame, frame_len); // might want to also add error checking here
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

    // this will fail if interrupt is sent before create_hidden_monitor_file() is called
    if (unlink(hidden_filepath) < 0) {
        perror("Deleting hidden .monitor_pid file");
    } 

    send_msg(MSG_EXIT, "Monitor received SIGINT - shutting down");

    _exit(0); // apparently, better and safer than exit(), it doesn't flush stdio buffers
}

// SIGUSR1 = report modification
void handle_sigusr1(int sig)
{
    (void) sig; // suppress warning, same as the same line of code in handle_sigint()

    send_msg(MSG_EVENT, "SIGUSR1 received - report modified (added or deleted)");
}

// SIGUSR2 = district modification
void handle_sigusr2(int sig)
{
    (void) sig; // suppress warning, same as the same line of code in handle_sigint()

    send_msg(MSG_EVENT, "SIGUSR2 received - district modified (added or deleted)");
}