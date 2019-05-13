#include <sys/types.h>
#include "log_writing.h"

int openLog(char* file)
{
    int fd = open(file, O_WRONLY | O_APPEND | O_CREAT);
    if (fd<0)
    {
        printf("Fail in opening file %s.\n", file);
        exit(3);
    }
}

void closeLog(int fd)
{
    fd=close(fd);
    if (fd<0)
    {
        perror("Failed closing logfile.\n");
        exit(3);
    }
}

void bankOfficeOpenLogWriting(int id)
{
    int fd=openLog(SERVER_LOGFILE);
    logBankOfficeOpen(fd, id, gettid());
    close(fd);
}

void bankOfficeCloseLogWriting(int id)
{
    int fd=openLog(SERVER_LOGFILE);
    logBankOfficeClose(fd, id, gettid());
    close(fd);
}

void accountCreationLogWriting(const bank_account_t *account, int id)
{
    int fd=openLog(SERVER_LOGFILE);
    logAccountCreation(fd, id, account);
    closeLog(fd);
}