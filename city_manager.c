#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>

#define MAX_STR 64

typedef struct {
    int id;
    char inspector[MAX_STR];
    float lat, lon;
    char category[MAX_STR];
    int severity;           
    time_t timestamp;
    char description[MAX_STR];
} Report;


void print_permissions(mode_t mode) {
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    return sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">") == 0)  return r->severity > val;
        if (strcmp(op, "<") == 0)  return r->severity < val;
    }
    if (strcmp(field, "category") == 0) {
        return strcmp(r->category, value) == 0;
    }
    return 1; 
}


void add_district(const char *name) {
    char path[256];
    if (mkdir(name, 0750) == -1) {
        perror("Error creating district");
        return;
    }
    sprintf(path, "%s/district.cfg", name);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd != -1) close(fd);

    sprintf(path, "%s/logged_district", name);
    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd != -1) close(fd);

    printf("District %s initialized.\n", name);
}

void add_report(const char *district, const char *user, int sev, const char *desc) {
    char path[256], link_path[256];
    sprintf(path, "%s/reports.dat", district);
    sprintf(link_path, "link_%s", district);

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if (fd < 0) { perror("File error"); return; }

    Report r = {.id = (int)time(NULL), .severity = sev, .timestamp = time(NULL)};
    strncpy(r.inspector, user, MAX_STR);
    strncpy(r.description, desc, MAX_STR);
    strncpy(r.category, "General", MAX_STR);

    write(fd, &r, sizeof(Report));
    close(fd);

    unlink(link_path);
    symlink(path, link_path);
    printf("Report added. ID: %d (Symlink: %s)\n", r.id, link_path);
}

void list_reports(const char *district, const char *filter_str) {
    char path[256];
    sprintf(path, "%s/reports.dat", district);
    int fd = open(path, O_RDONLY);
    if (fd < 0) { printf("No reports in %s\n", district); return; }

    char f[32], o[8], v[64];
    int has_f = (filter_str && parse_condition(filter_str, f, o, v));

    Report r;
    struct stat st;
    fstat(fd, &st);
    printf("File: reports.dat | Permissions: ");
    print_permissions(st.st_mode);
    printf(" | Size: %lld\n", (long long)st.st_size);

    while (read(fd, &r, sizeof(Report)) > 0) {
        if (!has_f || match_condition(&r, f, o, v)) {
            printf("[%d] Cat: %s | Sev: %d | Desc: %s\n", r.id, r.category, r.severity, r.description);
        }
    }
    close(fd);
}

void remove_report(const char *district, int id) {
    char path[256];
    sprintf(path, "%s/reports.dat", district);
    int fd = open(path, O_RDWR);
    if (fd < 0) return;

    struct stat st;
    fstat(fd, &st);
    Report *buf = malloc(st.st_size);
    Report cur;
    int count = 0, found = 0;

    while (read(fd, &cur, sizeof(Report)) > 0) {
        if (cur.id == id) { found = 1; continue; }
        buf[count++] = cur;
    }

    if (found) {
        lseek(fd, 0, SEEK_SET);
        write(fd, buf, count * sizeof(Report));
        ftruncate(fd, count * sizeof(Report));
        printf("Report %d deleted.\n", id);
    }
    free(buf);
    close(fd);
}


void create_snapshot(const char *dir, FILE *f) {
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *e;
    struct stat st;
    char path[1024];

    while ((e = readdir(d)) != NULL) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        if (lstat(path, &st) == -1) continue;

        fprintf(f, "%s %lld %o %ld\n", path, (long long)st.st_size, st.st_mode & 0777, (long)st.st_mtime);
        if (S_ISDIR(st.st_mode)) create_snapshot(path, f);
    }
    closedir(d);
}

void update_snapshot(const char *snap_path) {
    FILE *f = fopen(snap_path, "r");
    if (!f) return;
    char line[1024], path[512];
    long long sz; int md; long tm;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%s %lld %o %ld", path, &sz, &md, &tm) == 4) {
            struct stat st;
            if (lstat(path, &st) == -1) { printf("[MISSING]: %s\n", path); continue; }
            if (st.st_size != sz || (st.st_mode & 0777) != md || (long)st.st_mtime != tm) {
                printf("[MODIFIED]: %s\n", path);
            }
        }
    }
    fclose(f);
}


int main(int argc, char *argv[]) {
    char *role = NULL, *user = NULL;
    int op_idx = -1;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0) role = argv[++i];
        else if (strcmp(argv[i], "--user") == 0) user = argv[++i];
        else if (strncmp(argv[i], "--", 2) == 0) { op_idx = i; break; }
    }

    if (!role || !user || op_idx == -1) {
        printf("Usage: ./city_manager --role <r> --user <u> <cmd> <args>\n");
        return 1;
    }

    char *cmd = argv[op_idx];
    if (strcmp(cmd, "--add-district") == 0 && strcmp(role, "manager") == 0) {
        add_district(argv[op_idx + 1]);
    } else if (strcmp(cmd, "--report") == 0) {
        add_report(argv[op_idx + 1], user, atoi(argv[op_idx + 2]), argv[op_idx + 3]);
    } else if (strcmp(cmd, "--list") == 0) {
        list_reports(argv[op_idx + 1], (op_idx + 2 < argc) ? argv[op_idx + 2] : NULL);
    } else if (strcmp(cmd, "--remove-report") == 0 && strcmp(role, "manager") == 0) {
        remove_report(argv[op_idx + 1], atoi(argv[op_idx + 2]));
    } else if (strcmp(cmd, "--snapshot") == 0) {
        FILE *f = fopen("snapshot.txt", "w");
        if (f) { create_snapshot(".", f); fclose(f); printf("Snapshot saved.\n"); }
    } else if (strcmp(cmd, "--update") == 0) {
        update_snapshot("snapshot.txt");
    }

    return 0;
}