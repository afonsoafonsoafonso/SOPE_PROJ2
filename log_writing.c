#include "log_writing.h"
#include <pthread.h>

int openLog(char* file)
{
    int fd = open(file, O_WRONLY | O_APPEND | O_CREAT, 0666);
    if (fd<0)
    {
        printf("Fail in opening file %s.\n", file);
        exit(3);
    }
    return fd;
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
    logBankOfficeOpen(fd, id, pthread_self());
    close(fd);
}

void bankOfficeCloseLogWriting(int id)
{
    int fd=openLog(SERVER_LOGFILE);
    logBankOfficeClose(fd, id, pthread_self());
    close(fd);
}

void accountCreationLogWriting(const bank_account_t *account, int id)
{
    int fd=openLog(SERVER_LOGFILE);
    logAccountCreation(fd, id, account);
    closeLog(fd);
}

void requestSentLogWriting(const tlv_request_t *request, int id)
{
    int fd=openLog(USER_LOGFILE);
    lofRequest(fd, id, request);
    closeLog(fd);
}

void requestReceivedLogWriting(const tlv_request_t *request, int id)
{
    int fd=openLog(SERVER_LOGFILE);
    lofRequest(fd, id, request);
    closeLog(fd);
}

void replySentLogWriting(const tlv_reply_t *reply, int id)
{
    int fd=openLog(SERVER_LOGFILE);
    lofRequest(fd, id, reply);
    closeLog(fd);
}

void replyReceivedLogWriting(const tlv_reply_t *reply, int id)
{
    int fd=openLog(USER_LOGFILE);
    lofRequest(fd, id, reply);
    closeLog(fd);
}

void syncMechLogWriting(int id, sync_mech_op_t smo, sync_role_t role, int sid)
{
    int fd=openLog(USER_LOGFILE);
    logSyncMech(fd, id, smo, role, sid);
    close(fd);
}

void syncMechSemLogWriting(int id, sync_mech_op_t smo, sync_role_t role, int sid, int val)
{
    int fd=openLog(USER_LOGFILE);
    logSyncMechSem(fd, id, smo, role, sid, val);
    close(fd);
}

void delayLogWriting(int id, uint32_t delay_ms)
{
    int fd=openLog(USER_LOGFILE);
    logDelay(fd, id, delay_ms);
    close(fd);
}

void syncDelayLogWriting(int id, int sid, uint32_t delay_ms)
{
    int fd=openLog(USER_LOGFILE);
    logSyncDelay(fd, id, sid, delay_ms);
    close(fd);
}