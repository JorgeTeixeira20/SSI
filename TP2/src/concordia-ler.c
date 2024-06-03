#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/wait.h>

#define MAX_MESSAGE_SIZE 512

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s <mid> [group]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int flag = 0;

    char *mid = argv[1];

    char path[MAX_MESSAGE_SIZE * 2];
    if (argc == 2) {
        uid_t uid = geteuid();
        struct passwd *pwd = getpwuid(uid);
        if (pwd == NULL) {
            perror("getpwuid");
            exit(EXIT_FAILURE);
        }
        char *username = pwd->pw_name;

        snprintf(path, sizeof(path), "/var/concordia/users/%sMail/%s.txt", username, mid);
    } else {
        snprintf(path, sizeof(path), "/var/concordia/groups/%s/%s.txt", argv[2], mid);
        flag = 1;
    }
 
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char content[MAX_MESSAGE_SIZE];
    if (fgets(content, sizeof(content), file) == NULL) {
        if (!feof(file)) {
            perror("fgets");
            fclose(file);
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);

    char *copy = strdup(content);

    char *token = strtok(content, "$");
    int count = 0;
    char *third_section = NULL;

    while (token != NULL) {
        if (++count == 3) {
            third_section = token;
            break;
        }
        token = strtok(NULL, "$");
    }
    printf("%s\n", third_section);

    if (flag == 0) {
        if (copy != NULL && strlen(copy) > 0) {
            copy[strlen(copy) - 1] = '1';
        }

        file = fopen(path, "w");
        if (file == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        fputs(copy, file);
        fclose(file);
    }

    return 0;
}
