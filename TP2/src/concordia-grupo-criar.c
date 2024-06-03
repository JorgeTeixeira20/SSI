#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <grp.h>

#define GROUPS_FOLDER "/var/concordia/groups"
#define MAX_GROUP_NAME 100

int group_exists(const char *group_name) {
    FILE *fp = fopen(GROUPS_FOLDER, "r");
    if (fp == NULL) {
        if (errno == ENOENT) {
            return 0; 
        }
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char line[MAX_GROUP_NAME + 256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        char stored_group_name[MAX_GROUP_NAME];
        sscanf(line, "%[^:]", stored_group_name);
        if (strcmp(stored_group_name, group_name) == 0) {
            fclose(fp);
            return 1; 
        }
    }

    fclose(fp);
    return 0; 
}

int create_group_folder(const char *folder_path) {
    if (mkdir(folder_path, 0770) != 0) {
        perror("mkdir");
        return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <group_name>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *group_name = argv[1];
    if (strlen(group_name) > MAX_GROUP_NAME) {
        fprintf(stderr, "Group name too long. Maximum length is %d.\n", MAX_GROUP_NAME);
        exit(EXIT_FAILURE);
    }

    uid_t uid = geteuid();
    if (uid == (uid_t)-1) {
        perror("geteuid");
        exit(EXIT_FAILURE);
    }

    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }
    const char *username = pw->pw_name;

    if (group_exists(group_name)) {
        fprintf(stderr, "Group '%s' already exists.\n", group_name);
        exit(EXIT_FAILURE);
    }

    // Create the group first
    char command2[512];
    snprintf(command2, sizeof(command2), "sudo groupadd %s", group_name);
    int result2 = system(command2);
    if (result2 != 0) {
        fprintf(stderr, "Failed to create group %s\n", group_name);
        exit(EXIT_FAILURE);
    }

    char group_folder[256];
    snprintf(group_folder, sizeof(group_folder), "%s/%s", GROUPS_FOLDER, group_name);

    if (!create_group_folder(group_folder)) {
        fprintf(stderr, "Failed to create group folder '%s'.\n", group_folder);
        exit(EXIT_FAILURE);
    }

    // Set the correct group ownership
    struct group *grp = getgrnam(group_name);
    if (grp == NULL) {
        perror("getgrnam");
        exit(EXIT_FAILURE);
    }
    char command3[512];
    snprintf(command3, sizeof(command3), "sudo chown %d:%d %s", uid, grp->gr_gid, group_folder);
    int result3 = system(command3);
    if (result3 != 0) {
        fprintf(stderr, "Failed to change ownership of folder '%s'\n", group_folder);
        exit(EXIT_FAILURE);
    }
    char command4[512];
    snprintf(command4, sizeof(command4), "sudo chmod 0770 %s", group_folder);
    int result4 = system(command4);
    if (result4 != 0) {
        fprintf(stderr, "Failed to change permissions of folder '%s'\n", group_folder);
        exit(EXIT_FAILURE);
    }

    printf("Group '%s' created successfully by user '%s'.\n", group_name, username);

    // Add the user to the group
    char command[512];
    snprintf(command, sizeof(command), "sudo gpasswd -a %s %s", username, group_name);
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to add user %s to group %s\n", username, group_name);
        exit(EXIT_FAILURE);
    }

    printf("User %s added to group %s successfully.\n", username, group_name);

    return 0;
}