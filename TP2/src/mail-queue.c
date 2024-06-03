#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>

#define MAX_MESSAGE_SIZE 1024
#define MESSAGE_DIR "/home/concordia/queue"
#define MESSAGE_FILE_PREFIX "message"

int main() {
    if (mkdir(MESSAGE_DIR, 0700) == -1 && errno != EEXIST) {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }

    char message[MAX_MESSAGE_SIZE];
    fgets(message, MAX_MESSAGE_SIZE, stdin);

    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%s%d", MESSAGE_DIR, MESSAGE_FILE_PREFIX, getpid());

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (write(fd, message, strlen(message)) == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    printf("Message send successfully.\n"); 


    return 0;
}