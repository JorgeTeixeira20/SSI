#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>

#define MAX_MESSAGE_SIZE 512

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <mid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *mid = argv[1];

    char path[MAX_MESSAGE_SIZE*2];
    uid_t uid = geteuid(); 
    struct passwd *pwd = getpwuid(uid); 
    if (pwd == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }
    char *username = pwd->pw_name;

    snprintf(path, sizeof(path), "/var/concordia/users/%sMail/%s.txt", username, mid);

    // Remove the file
    if (remove(path) == -1) {
        perror("remove");
        exit(EXIT_FAILURE);
    }

    printf("Message removed successfully.\n");

    return 0;
}
