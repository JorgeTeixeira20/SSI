#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#define MAX_PATH_LENGTH 256
#define MAX_LINE_LENGTH 512

int extractIndex(const char *filename) {
    int index = 0;
    sscanf(filename, "%d_", &index);
    return index;
}

void listMessages(const char *folder_path, int showAll) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(folder_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    printf("%-7s %-24s %-20s %-10s\n", "Index", "Date", "Sender", "Size");
    int read = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            int index = extractIndex(entry->d_name);

            if (!read) {
                char file_path[2*MAX_PATH_LENGTH];
                snprintf(file_path, sizeof(file_path), "%s/%s", folder_path, entry->d_name);

                struct stat file_stat;
                if (stat(file_path, &file_stat) == -1) {
                    perror("stat");
                    continue;
                }

                char date[20];
                strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", localtime(&file_stat.st_atime));


                char sender[MAX_LINE_LENGTH];
                char message[MAX_LINE_LENGTH];

                FILE *file = fopen(file_path, "r");
                if (file == NULL) {
                    perror("fopen");
                    continue;
                }
                char *line = NULL;
                size_t len = 0;
                if (getline(&line, &len, file) != -1) {
                    char *token = strtok(line, "$");
                    if (token != NULL) {
                        strcpy(sender, token);
                        strtok(NULL, "$");
                        token = strtok(NULL, "$");
                        if (token != NULL) {
                            strcpy(message, token);
                        }
                        token = strtok(NULL, "$");
                        if (showAll != 1) {
                            if (strncmp(token, "1", 1) == 0) {
                                read = 1;
                            } else {
                                read = 0;
                            }
                        }
                    }
                }
                free(line);


                fclose(file);
                if (read != 1) {
                    printf("%-7d %-24s %-20s %-10ld\n", index, date, sender, strlen(message));
                }
            }
        }
    }
    closedir(dir);
}

void getUserGroups(const char *username) {
    struct passwd *pw = getpwnam(username);
    if (pw == NULL) {
        perror("getpwnam");
        exit(EXIT_FAILURE);
    }

    int ngroups = 0;
    gid_t *groups = NULL;

    if (getgrouplist(username, pw->pw_gid, NULL, &ngroups) == -1) {
        groups = (gid_t *)malloc(ngroups * sizeof(gid_t));
        if (groups == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        if (getgrouplist(username, pw->pw_gid, groups, &ngroups) == -1) {
            perror("getgrouplist");
            exit(EXIT_FAILURE);
        }
    }

    char group_path[MAX_LINE_LENGTH];
    for (int i = 0; i < ngroups; ++i) {
        struct group *grp = getgrgid(groups[i]);
        if (grp != NULL) {
            snprintf(group_path, sizeof(group_path), "/var/concordia/groups/%s", grp->gr_name);
            if (access(group_path, F_OK) == -1) {
                continue;
            } 
            printf("Group: %s\n", grp->gr_name);
            listMessages(group_path, 1);
            putchar('\n');
        }
    }

    free(groups);
}


int main(int argc, char *argv[]) {
    int showAll;
    if (argc == 2 && strcmp(argv[1], "-a") == 0) {
        showAll = 1;
    } else if (argc == 2 && strcmp(argv[1], "-g") == 0)  {
        showAll = 2;
    } else if (argc == 1) {
        showAll = 0;
    } else {
        fprintf(stderr, "Usage: %s [flag]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    uid_t uid = geteuid(); 
    struct passwd *pwd = getpwuid(uid); 
    if (pwd == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }
    char *username = pwd->pw_name;

    if (showAll == 1 || showAll == 0) {
        char folder_path[MAX_PATH_LENGTH];
        snprintf(folder_path, sizeof(folder_path), "/var/concordia/users/%sMail", username);
        listMessages(folder_path, showAll);
    } else {
        getUserGroups(username);
    }

    return 0;
}