#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "constants.h"

void produceSha(const char* toEncrypt, char* encrypted)
{
    int fd[2];
    pipe(fd);
    int pid=fork();
    if (pid<0)
    {
        perror("Error ocured in fork.\n");
        exit(50);
    }
    if (pid==0)
    {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);

        int fd2[2];
        pipe(fd2);
        pid=fork();
        if (pid<0)
        {
            perror("Error ocured in fork.\n");
            exit(50);
        }
        if (pid==0)
        {
            close(fd2[0]);
            dup2(fd2[1], STDOUT_FILENO);
            execlp("echo", "echo", toEncrypt, NULL);
            perror("Error on call to echo.\n");
            exit(50);
        }
        
        close(fd2[1]);
        wait(NULL);
        dup2(fd2[0], STDIN_FILENO);
        execlp("sha256sum", "sha256sum", NULL);
        perror("Error on call to sha256sum.\n");
        exit(50);
    }
    close(fd[1]);
    read(fd[0], encrypted, HASH_LEN+1);
    wait(NULL);
}

// ----------------------------- TO COMPLETE -----------------------------------
void produceSalt(char* salt)
{
    strcpy(salt, "salt");
}

