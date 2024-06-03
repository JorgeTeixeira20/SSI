#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <sys/stat.h>

#define MAX_GROUP_NAME 100

int is_user_in_system(const char *username) {
    struct passwd *pw = getpwnam(username);
    return (pw != NULL);
}

int is_user_group_owner(const char *group_name, const char *username) {
    char group_folder[256];
    snprintf(group_folder, sizeof(group_folder), "/var/concordia/groups/%s", group_name);

    struct stat st;
    if (stat(group_folder, &st) != 0) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    struct passwd *pw = getpwuid(st.st_uid);
    if (pw == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }

    return (strcmp(pw->pw_name, username) == 0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <group_name> <username>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *group_name = argv[1];
    const char *username_to_add = argv[2];

    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }
    const char *current_username = pw->pw_name;

    if (!is_user_in_system(username_to_add)) {
        fprintf(stderr, "User '%s' does not exist in the system.\n", username_to_add);
        exit(EXIT_FAILURE);
    }

    if (!is_user_group_owner(group_name, current_username)) {
        fprintf(stderr, "You are not the owner of the group '%s'.\n", group_name);
        exit(EXIT_FAILURE);
    }

    char command[256];
    snprintf(command, sizeof(command), "sudo gpasswd -a %s %s", username_to_add, group_name);
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to add user '%s' to group '%s'.\n", username_to_add, group_name);
        exit(EXIT_FAILURE);
    }

    printf("User '%s' added to group '%s' successfully.\n", username_to_add, group_name);
    return 0;
}
