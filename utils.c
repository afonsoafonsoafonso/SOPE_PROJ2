#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <errno.h>
#include "constants.h"
#include <time.h>

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
    int n;
    n=read(fd[0], encrypted, HASH_LEN+1);
    encrypted[n]='\0';
    wait(NULL);
}

// ----------------------------- TO COMPLETE -----------------------------------
void produceSalt(char* salt)
{
    char* temp;
    strcpy(salt, "salt");
    sprintf(temp,"%ld",clock());
    strcat(salt, temp);
    sprintf(temp,"%d",getpid());
    strcat(salt, temp);
    while (strlen(salt)<64){
        sprintf(temp,"%d",(rand() % 10));
        strcat(salt, temp);}
}

void createFifo(char* fifo_name)
{
    if (mkfifo(fifo_name,0660)<0 && errno!=EEXIST)
    {
        printf("Can't create FIFO %s\n", fifo_name);
        exit(2);
    }
}

int openReadFifo(char* fifo_name, int * fd_dummy)
{
    int fd;    
    if ((fd=open(fifo_name, O_RDONLY | O_NONBLOCK)) <0)
    {
        printf("Can't open FIFO %s\n", fifo_name);
        exit(2);
    }
    *fd_dummy=open(fifo_name,O_WRONLY);
    return fd;
}

int openWriteFifo(char* fifo_name)
{
    int fd;    
    if ((fd=open(fifo_name, O_WRONLY | O_APPEND)) <0)
    {
        printf("Can't open FIFO %s\n", fifo_name);
        exit(2);
    }
    return fd;
}

void closeUnlinkFifo(char* fifo_name, int fd, int fd_dummy)
{
    close(fd);
    close(fd_dummy);
    if (unlink(fifo_name)<0)
    {
        printf("Error when destroying FIFO %s\n", fifo_name);
        exit(0); 
    }
}
