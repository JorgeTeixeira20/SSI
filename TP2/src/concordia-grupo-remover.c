#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#define GROUPS_FOLDER "/var/concordia/groups"
#define MAX_GROUP_NAME 100

int remove_directory_recursively(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;
        r = 0;
        while (!r && (p = readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
                continue;
            }

            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;
                snprintf(buf, len, "%s/%s", path, p->d_name);

                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode)) {
                        r2 = remove_directory_recursively(buf);
                    } else {
                        r2 = unlink(buf);
                    }
                }

                free(buf);
            }

            r = r2;
        }
        closedir(d);
    }

    if (!r) {
        r = rmdir(path);
    }

    return r;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <group_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *group_name = argv[1];

    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }

    const char *username = pw->pw_name;


    char group_folder[256];
    snprintf(group_folder, sizeof(group_folder), "%s/%s", GROUPS_FOLDER, group_name);

    struct stat st;
    if (stat(group_folder, &st) != 0) {
        perror("stat");
        exit(EXIT_FAILURE);
    }


    if (st.st_uid != uid) {
        fprintf(stderr, "You are not the owner of the group '%s'.\n", group_name);
        exit(EXIT_FAILURE);
    }

    // Executar o comando para remover o grupo do sistema operacional
    char command[256];
    snprintf(command, sizeof(command), "sudo groupdel %s", group_name);
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to remove group '%s' from the system.\n", group_name);
        exit(EXIT_FAILURE);
    }

    // Remover a pasta do grupo
    if (remove_directory_recursively(group_folder) != 0) {
        perror("remove_directory_recursively");
        exit(EXIT_FAILURE);
    }

    printf("Group '%s' and its folder removed successfully by user '%s'.\n", group_name, username);

    return 0;
}