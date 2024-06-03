#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define MAX_LINE_LENGTH 512
#define MAX_USERNAME_LENGTH 100
#define MAX_PATH_LENGTH 256
#define FIFO_PATH "/var/concordia/mailfifo"

int extractNumber(const char *filename) {
    int number = 0;
    sscanf(filename, "%d.txt", &number);
    return number;
}

int getHighestNumber(const char *directoryPath) {
    DIR *dir;
    struct dirent *entry;
    int highestNumber = 0;
    int fileCount = 0;

    dir = opendir(directoryPath);
    if (dir == NULL) {
        perror("opendir1");
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') { 
            fileCount++;
            int number = extractNumber(entry->d_name);
            if (number > highestNumber) {
                highestNumber = number;
            }
        }
    }
    closedir(dir);

    if (fileCount == 0) {
        return 0;
    }
    return highestNumber;
}

void createFile(const char *content, const char *username) {
    char folder_path[MAX_PATH_LENGTH];
    char file_path[2*MAX_PATH_LENGTH];

    snprintf(folder_path, sizeof(folder_path), "/var/concordia/groups/%s", username);

    struct passwd *pwd = NULL;
    struct group *grp = NULL;
    if (access(folder_path, F_OK) == -1) {
        snprintf(folder_path, sizeof(folder_path), "/var/concordia/users/%sMail", username);
        pwd = getpwnam(username);
    } else {
        grp = getgrnam(username);
    }

    int tam = getHighestNumber(folder_path);
    
    snprintf(file_path, sizeof(file_path), "%s/%d.txt", folder_path, tam+1);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("fopen1");
        return;
    }

    fprintf(file, "%s$0", content);
    fclose(file);

    printf("File created at: %s\n", file_path);

    //struct passwd *pwd = getpwnam(username);
    if (pwd != NULL) {
        if (pwd == NULL) {
            perror("getpwnam");
            return;
        }

        if (chown(file_path, pwd->pw_uid, pwd->pw_gid) == -1) {
            perror("chown");
            return;
        }

        if (chmod(file_path, S_IRUSR | S_IWUSR) == -1) {
            perror("chmod");
            return;
        }
    } else {
        if (grp == NULL) {
            perror("getgrnam");
            return;
        }

        if (chown(file_path, -1, grp->gr_gid) == -1) {
            perror("chown");
            return;
        }

        if (chmod(file_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1) {
            perror("chmod");
            return;
        }
    }
}

int main() {
    int fifo_fd;
    char content[MAX_LINE_LENGTH];

    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    while (1) {
        ssize_t bytes_read = read(fifo_fd, content, sizeof(content));
        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue; 
            } else {
                perror("read");
                exit(EXIT_FAILURE);
            }
        } else if (bytes_read == 0) {
            sleep(1);
            continue;
        } else {
            content[bytes_read] = '\0';
            char *copy = strdup(content);
            if (copy == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(EXIT_FAILURE);
            }

            strtok(content, "$");
            char *dest = strtok(NULL, "$");

            createFile(copy, dest);
            free(copy); 
        }
    }
    close(fifo_fd);

    return EXIT_SUCCESS;
}
