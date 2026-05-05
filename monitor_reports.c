// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <sys/stat.h>
// #include <sys/wait.h>
#include "city_manager.c"

int keep_running = 1;

void handle_sigint(int sig) {
    keep_running = 0;
}

void handle_sigusr1(int sig) {
    char* msg = "[MONITOR] A new report has been added.\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}


int main(){
    const char *pathname = "Documents/ProjectOS/.monitor_pid";
    int fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0){
        perror("Failed to open file\n");
        return 1;
    }
    int pid = getpid();
    dprintf(fd, "%d\n", pid);
    close(fd);

    printf("[MONITOR] Monitor process started with PID: %d\n", pid);
    printf("[MONITOR] PID written to %s\n", pathname);
    printf("[MONITOR] Waiting for signals...\n");

    struct sigaction sa_int, sa_usr1;

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    if(sigaction(SIGINT, &sa_int, NULL) == -1){
        perror("Failed to set SIGINT handler\n");
        return 1;
    }

    sa_usr1.sa_handler = SIGUSR1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    if(sigaction(SIGUSR1, &sa_usr1, NULL) == -1){
        perror("Failed to set SIGUSR1 handler\n");
        return 1;
    }

    while(keep_running){
        pause();
    }

    if(unlink(pathname) == 0){
        printf("[MONITOR] .monitor_pid deleted succesfully\n");
        return 0;
    }
    else{
        perror("[MONITOR] Failed to remove PID file\n");
        return 1;
    }


}