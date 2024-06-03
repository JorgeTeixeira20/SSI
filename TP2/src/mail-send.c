#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_PATH_LENGTH 256
#define MAX_MESSAGE_SIZE 512
#define FIFO_PATH "/var/concordia/mailfifo"

int send_message(const char *dir_path, const char *file_name) {
    char file_path[MAX_PATH_LENGTH];
    snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, file_name);

    int fifo_fd = open(FIFO_PATH, O_WRONLY);
    if (fifo_fd == -1) {
        perror("open");
        return -1;
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("fopen");
        close(fifo_fd);
        return -1;
    }

    char buffer[MAX_MESSAGE_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (write(fifo_fd, buffer, bytes_read) == -1) {
            perror("write");
            fclose(file);
            close(fifo_fd);
            return -1;
        }
    }

    printf("Message sent successfully: %s\n", file_path);

    fclose(file);
    close(fifo_fd);

    if (remove(file_path) == -1) {
        perror("remove");
        return -1;
    }

    printf("File removed successfully: %s\n", file_path);

    return 0;
}

int main() {
    char queuePath[] = "/home/concordia/queue";
    DIR *dir;
    struct dirent *entry;

    dir = opendir(queuePath); 
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    while (1) {
        rewinddir(dir); 
        
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            if (send_message(queuePath, entry->d_name) != 0) {
                fprintf(stderr, "Failed to send message from file: %s/%s\n", queuePath, entry->d_name);
            }
        }
        sleep(1);
    }
    closedir(dir);

    return 0;
}
