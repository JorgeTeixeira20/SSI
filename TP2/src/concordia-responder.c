#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>

#define MAX_MESSAGE_SIZE 512

char *getSenderFromFile(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        perror("fopen");
        return NULL;
    }

    char content[MAX_MESSAGE_SIZE];
    if (fgets(content, sizeof(content), file) == NULL) {
        if (feof(file)) {
            strcpy(content, "");
        } else {
            perror("fgets");
            fclose(file);
            return NULL;
        }
    }
    fclose(file);

    char *sender = strtok(content, "$");

    return sender != NULL ? strdup(sender) : NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <mid> <msg>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *mid = argv[1];
    char *msg = argv[2];

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipefd[1]);

        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            close(pipefd[0]);
            exit(EXIT_FAILURE);
        }

        execl("mail-queue", "mail-queue", (char *)NULL);
        
        perror("execl");
        exit(EXIT_FAILURE);
    } else {
        close(pipefd[0]);


        char message[MAX_MESSAGE_SIZE*2];
        uid_t uid = geteuid(); 
        struct passwd *pwd = getpwuid(uid); 
        if (pwd == NULL) {
            perror("getpwuid");
            exit(EXIT_FAILURE);
        }
        char *username = pwd->pw_name;

        snprintf(message, sizeof(message), "/var/concordia/users/%sMail/%s.txt", username, mid);

        snprintf(message, sizeof(message), "%s$%s$%s", username, getSenderFromFile(message), msg);
        //snprintf(message, sizeof(message), "%s$%s", dest, msg);

        if (write(pipefd[1], message, strlen(message)) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        close(pipefd[1]);
                
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
