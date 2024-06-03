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
    const char *username_to_remove = argv[2];

    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }
    const char *current_username = pw->pw_name;

    if (strcmp(current_username, username_to_remove) == 0) {
        fprintf(stderr, "You cannot remove yourself from the group '%s'.\n", group_name);
        exit(EXIT_FAILURE);
    }

    if (!is_user_in_system(username_to_remove)) {
        fprintf(stderr, "User '%s' does not exist in the system.\n", username_to_remove);
        exit(EXIT_FAILURE);
    }

    if (!is_user_group_owner(group_name, current_username)) {
        fprintf(stderr, "You are not the owner of the group '%s'.\n", group_name);
        exit(EXIT_FAILURE);
    }

    char command[256];
    snprintf(command, sizeof(command), "sudo gpasswd -d %s %s", username_to_remove, group_name);
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to remove user '%s' from group '%s'.\n", username_to_remove, group_name);
        exit(EXIT_FAILURE);
    }

    printf("User '%s' removed from group '%s' successfully.\n", username_to_remove, group_name);
    return 0;
}
