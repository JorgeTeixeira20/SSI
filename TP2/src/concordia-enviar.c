#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/wait.h>
#include <grp.h>

#define MAX_MESSAGE_SIZE 512

int userBelongsToGroup(const char *groupname) {
    uid_t uid = geteuid(); 
    struct passwd *pwd = getpwuid(uid); 
    if (pwd == NULL) {
        perror("getpwuid");
        exit(EXIT_FAILURE);
    }
    char *username = pwd->pw_name;
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

    for (int i = 0; i < ngroups; ++i) {
        struct group *grp = getgrgid(groups[i]);
        if (grp != NULL && strcmp(grp->gr_name, groupname) == 0) {
            free(groups);
            return 1;
        }
    }
    free(groups);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <dest> <msg>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *dest = argv[1];
    char *msg = argv[2];

    char user_path[MAX_MESSAGE_SIZE];
    char group_path[MAX_MESSAGE_SIZE];
    snprintf(user_path, sizeof(user_path), "/var/concordia/users/%sMail", dest);
    snprintf(group_path, sizeof(group_path), "/var/concordia/groups/%s", dest);

    if (access(user_path, F_OK) == -1) {
        if (access(group_path, F_OK) == -1) {
            printf("User or Group does not exist.\n");
            return 1;
        } else if (userBelongsToGroup(dest) == 0) {
            printf("User doesnt belong to group.\n");
            return 1;
        }
    }

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
        snprintf(message, sizeof(message), "%s$%s$%s", username, dest, msg);
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
