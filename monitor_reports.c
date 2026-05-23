#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

int keep_running = 1;

static void write_message(int fd, const char *type, const char *message) {
    char buf[256];
    int len = snprintf(buf, sizeof(buf), "%s:%s\n", type, message);
    if (len > 0) {
        write(fd, buf, len);
    }
}

static int read_existing_monitor_pid(pid_t *pid_out) {
    const char *pathname = "./.monitor_pid";
    int fd = open(pathname, O_RDONLY);
    if (fd < 0) {
        return -1;
    }

    char buf[64];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) {
        return -1;
    }

    buf[n] = '\0';
    pid_t pid = (pid_t)atoi(buf);
    if (pid <= 0) {
        return -1;
    }

    *pid_out = pid;
    return 0;
}

void handle_sigint(int sig) {
    (void)sig;
    keep_running = 0;
}

void handle_sigusr1(int sig) {
    (void)sig;
    const char *msg = "NOTICE:A new report has been added.";
    write(STDOUT_FILENO, msg, strlen(msg));
    write(STDOUT_FILENO, "\n", 1);
}

int main(void) {
    const char *pathname = "./.monitor_pid";
    pid_t existing_pid;
    if (read_existing_monitor_pid(&existing_pid) == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Monitor already running with PID %d", existing_pid);
        write_message(STDOUT_FILENO, "ERROR", msg);
        return 1;
    }

    int fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        write_message(STDOUT_FILENO, "ERROR", "Failed to create PID file");
        return 1;
    }

    pid_t pid = getpid();
    dprintf(fd, "%d\n", (int)pid);
    close(fd);

    write_message(STDOUT_FILENO, "INFO", "Monitor process started");
    write_message(STDOUT_FILENO, "INFO", "PID written to .monitor_pid");
    write_message(STDOUT_FILENO, "INFO", "Waiting for signals...");

    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        write_message(STDOUT_FILENO, "ERROR", "Failed to set SIGINT handler");
        return 1;
    }

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        write_message(STDOUT_FILENO, "ERROR", "Failed to set SIGUSR1 handler");
        return 1;
    }

    while (keep_running) {
        pause();
    }

    if (unlink(pathname) == 0) {
        write_message(STDOUT_FILENO, "EXIT", "PID file deleted successfully");
        return 0;
    }

    write_message(STDOUT_FILENO, "ERROR", "Failed to remove PID file");
    return 1;
}
