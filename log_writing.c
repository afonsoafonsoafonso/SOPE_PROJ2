#include "log_writing.h"

void accountCreationHandler(const bank_account_t *account)
{
    int fd = open(SERVER_LOGFILE, O_WRONLY | O_APPEND | O_CREAT);
    if (fd<0)
    {
        perror("Fail in server logfile opening.");
        exit(3);
    }
    logAccountCreation(fd, 0, account);
    fd=close(fd);
    if (fd<0)
    {
        perror("Fail in server logfile closing.");
        exit(3);
    }
}