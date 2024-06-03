#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#define MAX_GROUP_NAME 100

int is_user_in_group(const char *group_name, const char *username) {
    struct group *grp = getgrnam(group_name);
    if (grp == NULL) {
        fprintf(stderr, "Group '%s' not found.\n", group_name);
        return 0;
    }

    char **members = grp->gr_mem;
    while (*members) {
        if (strcmp(*members, username) == 0) {
            return 1;
        }
        members++;
    }

    return 0;
}

void list_group_members(const char *group_name) {
    char command[256];
    snprintf(command, sizeof(command), "getent group %s", group_name);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    if (fgets(line, sizeof(line), fp) != NULL) {
        char *token = strtok(line, ":");
        int field_count = 0;
        while (token != NULL) {
            field_count++;
            if (field_count == 4) {
                // The fourth field contains the list of users
                if (strlen(token) > 0) {
                    printf("Members of group '%s': %s\n", group_name, token);
                } else {
                    printf("Group '%s' has no members.\n", group_name);
                }
                break;
            }
            token = strtok(NULL, ":");
        }
    } else {
        printf("Group '%s' not found.\n", group_name);
    }

    pclose(fp);
    
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

    if (!is_user_in_group(group_name, username)) {
        fprintf(stderr, "You are not a member of the group '%s'.\n", group_name);
        exit(EXIT_FAILURE);
    }

    list_group_members(group_name);

    return 0;
}

