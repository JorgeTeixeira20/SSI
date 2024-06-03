#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>

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

    mode_t mode = S_IRWXU; // Set permissions for the owner (user)

    if (mkdir(folder_path, mode) != 0) {
        perror("mkdir");
        exit(EXIT_FAILURE);
    } else {
        printf("Directory %s created successfully!\n", folder_name);
    }

    return 0;
}
