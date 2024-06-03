#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>

void remove_directory(const char *folder_path) {
    DIR *d = opendir(folder_path);
    if (!d) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *dir;
    while ((dir = readdir(d)) != NULL) {
        if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", folder_path, dir->d_name);
            if (remove(path) != 0) {
                perror("remove");
                exit(EXIT_FAILURE);
            }
        }
    }
    closedir(d);

    if (remove(folder_path) != 0) {
        perror("remove");
        exit(EXIT_FAILURE);
    } else {
        printf("Directory %s removed successfully!\n", folder_path);
    }
}

int main() {
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }

    char *username = pw->pw_name;
    char folder_name[100];
    snprintf(folder_name, sizeof(folder_name), "%sMail", username);

    char folder_path[256];
    snprintf(folder_path, sizeof(folder_path), "/var/concordia/users/%s", folder_name);

    remove_directory(folder_path);

    return 0;
}