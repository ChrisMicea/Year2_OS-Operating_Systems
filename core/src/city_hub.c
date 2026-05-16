#include "../inc/commons.h"
#include "../inc/utils.h"

// monitor_reports sends lines of the form:
// TYPE:LENGTH:payload\n
// TYPE is one of: ERROR | EVENT | EXIT
// LENGTH is the decimal byte count of the payload (not counting \n)
// hub_mon reads these and pretty-prints them.

#define MSG_BUF_SIZE 512  // max frame size we ever expect

static pid_t hub_mon_pid = -1;  // city_hub tracks hub_mon so it can wait later

static void run_hub_mon(int pipe_read_fd, int pipe_write_fd);
static int start_monitor();
static void calculate_scores(char **districts, int count);
static void repl();

int main(void)
{
    printf("city_hub started. Type 'help' for available commands.\n");
    repl();
    return 0;
}

static void repl()
{
    char line[256];
 
    while (1) {
        printf("hub> ");
        fflush(stdout);
 
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nEOF received - exiting city_hub.\n");
            break;
        }
 
        line[strcspn(line, "\n")] = '\0';
 
        if (strcmp(line, "start_monitor") == 0) {
            if (hub_mon_pid > 0) {
                int status;
                if (waitpid(hub_mon_pid, &status, WNOHANG) == 0) {
                    printf("[hub] Monitor is already running (hub_mon PID %d).\n", hub_mon_pid);
                    continue;
                }
                hub_mon_pid = -1; // hub_mon already exited, allow restart
            }
            start_monitor();
        }
        else if (strncmp(line, "calculate_scores", 16) == 0) {
            char* token = strtok(line + 16, " ");
            char* districts[MAX_DISTRICTS];
            int count = 0;
            while (token && (count < MAX_DISTRICTS)) {
                districts[count++] = token;
                token = strtok(NULL, " ");
            }
            if (count == 0)
                printf("[hub] Usage: calculate_scores <district1> [district2 ...]\n");
            else
                calculate_scores(districts, count);
        }
        else if (strcmp(line, "help") == 0) {
            print_usage_hub();
        }
        else if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0) {
            printf("[hub] Shutting down.\n");
            break;
        }
        else if (line[0] != '\0') {
            printf("[hub] Unknown command: '%s'\n", line);
            print_usage_hub();
        }
    }
}


static int start_monitor()
{
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("[hub] pipe() failed");
        return -1;
    }
 
    pid_t pid = fork();
    if (pid < 0) {
        perror("[hub] fork() failed");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }
 
    if (pid == 0) {
        // child = hub_mon: takes ownership of both pipe ends
        run_hub_mon(pipefd[0], pipefd[1]);
        _exit(0);
    }
 
    // parent = city_hub: doesn't use the pipe, hub_mon owns it
    close(pipefd[0]);
    close(pipefd[1]);
 
    hub_mon_pid = pid;
    printf("[hub] hub_mon started (PID %d).\n", pid);
    return 0;
}

// hub_mon: fork + exec monitor_reports with its stdout -> write_fd,
// then read framed messages from read_fd and pretty-print them.
static void run_hub_mon(int read_fd, int write_fd)
{
    pid_t mon_pid = fork();
    if (mon_pid < 0) {
        perror("[hub_mon] fork() failed");
        _exit(1);
    }
 
    if (mon_pid == 0) {
        // grandchild == monitor_reports
        close(read_fd); // monitor only writes
        if (dup2(write_fd, STDOUT_FILENO) == -1) {
            perror("[hub_mon] dup2() failed");
            _exit(1);
        }
        close(write_fd); // duplicated into stdout, original fd no longer needed
        execlp("./monitor_reports", "monitor_reports", NULL);
        perror("[hub_mon] execlp() failed");
        _exit(1);
    }
 
    // hub_mon reader loop
    close(write_fd); // hub_mon only reads
 
    printf("[hub_mon] monitor_reports launched (PID %d). Listening...\n", mon_pid);
    // fflush(stdout);
 
    char buf[MSG_BUF_SIZE];
 
    while (1) {
        ssize_t nBytesRead = read(read_fd, buf, sizeof(buf) - 1);
 
        if (nBytesRead <= 0) {
            if (nBytesRead < 0)
                perror("[hub_mon] read()");
            printf("[hub_mon] Pipe closed - monitor has ended unexpectedly.\nBytesRead");
            break;
        }
 
        buf[nBytesRead] = '\0';
        // strip trailing newline left by monitor's send_msg
        buf[strcspn(buf, "\n")] = '\0';
 
        // parse TYPE:LENGTH:payload
        char *c1 = strchr(buf, ':');
        char *c2 = c1 ? strchr(c1 + 1, ':') : NULL;
 
        if (!c1 || !c2) {
            printf("[hub_mon] (malformed) %s\n", buf);
            continue;
        }
 
        // null-terminate the type field and point payload past the second colon
        *c1 = '\0';
        const char *type = buf;
        const char *payload = c2 + 1;
 
        if (strcmp(type, "EVENT") == 0) {
            printf("[hub_mon] Received from monitor_reports: %s\n", payload);
        } 
        else if (strcmp(type, "ERROR") == 0) {
            printf("[hub_mon] *** ERROR from monitor_reports: %s\n", payload);
        } 
        else if (strcmp(type, "EXIT") == 0) {
            printf("[hub_mon] Received from monitor_reports: %s\n", payload);
            printf("[hub_mon] Monitor has ended. hub_mon shutting down.\n");
            break;
        } 
        else {
            printf("[hub_mon] (unknown type '%s') %s\n", type, payload);
        }
    }
 
    close(read_fd);
 
    int status;
    waitpid(mon_pid, &status, 0);
    printf("[hub_mon] monitor_reports reaped. hub_mon exiting.\n");
    // fflush(stdout);
}

// For each district, spawn a scorer process with its stdout piped back,
// collect the output, and print a combined workload report.
static void calculate_scores(char **districts, int count)
{
    // TODO: update the logic so that when any of the pipe or fork calls fail, program exits
    int pipefd[count][2];
    pid_t pids[count];
 
    // spawn one scorer per district
    for (int i = 0; i < count; i++) {
        if (pipe(pipefd[i]) == -1) {
            perror("[hub] pipe() failed");
            pipefd[i][0] = pipefd[i][1] = -1;
            pids[i] = -1;
            continue; // temporary logic so I don't have to go back and close all previous pipes and end the program
        }
 
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("[hub] fork() failed");
            close(pipefd[i][0]);
            close(pipefd[i][1]);
            pids[i] = -1;
            continue; // temporary logic so I don't have to go back and close all previous pipes and end the program
        }
 
        // child = scorer
        if (pids[i] == 0) {
            // close read end
            close(pipefd[i][0]); // scorer only writes

            if (dup2(pipefd[i][1], STDOUT_FILENO) == -1) {
                perror("[hub] dup2() failed");
                _exit(1);
            }
            close(pipefd[i][1]);
            execlp("./scorer", "scorer", districts[i], NULL);
            perror("[hub] execlp scorer failed");
            _exit(1);
        }
 
        // parent: close write end, will read from read end below
        close(pipefd[i][1]);
    }
 
    // collect and print output from each scorer
    printf("\n=== Workload Report ===\n");
    char buf[1024];
    for (int i = 0; i < count; i++) {
        if (pids[i] == -1)
            continue;
 
        ssize_t n;
        while ((n = read(pipefd[i][0], buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("%s", buf);
        }
        close(pipefd[i][0]);
 
        int status;
        waitpid(pids[i], &status, 0);
    }
    printf("======================\n\n");
}