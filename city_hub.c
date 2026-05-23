#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>

#define MAX_DISTRICT 128
#define MAX_LINE 1024

int directory_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

void run_hub_monitor() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        _exit(1);
    }

    pid_t monitor_pid = fork();
    if (monitor_pid < 0) {
        perror("fork");
        _exit(1);
    }

    if (monitor_pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            _exit(1);
        }
        close(pipefd[1]);
        execl("./monitor_reports", "monitor_reports", (char *)NULL);
        perror("execl");
        _exit(127);
    }

    close(pipefd[1]);
    char buf[512];
    char line[MAX_LINE];
    int line_len = 0;

    while (1) {
        ssize_t r = read(pipefd[0], buf, sizeof(buf));
        if (r <= 0) break;

        for (ssize_t i = 0; i < r; ++i) {
            if (line_len < (int)sizeof(line) - 1) {
                line[line_len++] = buf[i];
            }
            if (buf[i] == '\n') {
                line[line_len] = '\0';
                write(STDOUT_FILENO, line, line_len);
                if (strncmp(line, "EXIT:", 5) == 0 || strncmp(line, "ERROR:", 6) == 0) {
                    const char *notice = "MONITOR HUB: monitor ended.\n";
                    write(STDOUT_FILENO, notice, strlen(notice));
                }
                line_len = 0;
            }
        }
    }

    if (line_len > 0) {
        line[line_len] = '\0';
        write(STDOUT_FILENO, line, line_len);
    }

    close(pipefd[0]);
    int status = 0;
    waitpid(monitor_pid, &status, 0);
    _exit(0);
}

void start_monitor_command() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        run_hub_monitor();
        _exit(0);
    }

    printf("Monitor hub started as PID %d\n", pid);
}

void calculate_scores_command(char **districts, int count) {
    if (count <= 0) {
        printf("Usage: calculate_scores <district1> [district2 ...]\n");
        return;
    }

    pid_t pids[MAX_DISTRICT];
    int pipes[MAX_DISTRICT];
    char names[MAX_DISTRICT][MAX_DISTRICT];
    int active = 0;

    for (int i = 0; i < count; ++i) {
        const char *district = districts[i];
        if (!directory_exists(district)) {
            printf("Warning: district '%s' does not exist. Skipping.\n", district);
            continue;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            continue;
        }

        if (pid == 0) {
            close(pipefd[0]);
            if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }
            close(pipefd[1]);
            execl("./scorer", "scorer", district, (char *)NULL);
            perror("execl");
            _exit(127);
        }

        close(pipefd[1]);
        pids[active] = pid;
        pipes[active] = pipefd[0];
        strncpy(names[active], district, MAX_DISTRICT - 1);
        names[active][MAX_DISTRICT - 1] = '\0';
        active++;
    }

    if (active == 0) {
        printf("No valid districts to score.\n");
        return;
    }

    printf("--- Combined workload report ---\n");
    for (int i = 0; i < active; ++i) {
        printf("District: %s\n", names[i]);
        char outbuf[512];
        ssize_t r;
        while ((r = read(pipes[i], outbuf, sizeof(outbuf))) > 0) {
            write(STDOUT_FILENO, outbuf, r);
        }
        close(pipes[i]);
        int status;
        waitpid(pids[i], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            printf("(Scorer for district %s exited with status %d)\n", names[i], WIFEXITED(status) ? WEXITSTATUS(status) : -1);
        }
        printf("---\n");
    }
}

int main(void) {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("city_hub> ");
        fflush(stdout);
        ssize_t readlen = getline(&line, &len, stdin);
        if (readlen <= 0) break;
        if (readlen > 0 && line[readlen - 1] == '\n') {
            line[readlen - 1] = '\0';
        }

        char *token = strtok(line, " \t");
        if (!token) continue;

        if (strcmp(token, "start_monitor") == 0) {
            start_monitor_command();
        } else if (strcmp(token, "calculate_scores") == 0) {
            char *districts[MAX_DISTRICT];
            int count = 0;
            while ((token = strtok(NULL, " \t")) != NULL && count < MAX_DISTRICT) {
                districts[count++] = token;
            }
            calculate_scores_command(districts, count);
        } else if (strcmp(token, "quit") == 0 || strcmp(token, "exit") == 0) {
            break;
        } else {
            printf("Unknown command: %s\n", token);
            printf("Available commands: start_monitor, calculate_scores <districts>, exit\n");
        }
    }

    free(line);
    return 0;
}
