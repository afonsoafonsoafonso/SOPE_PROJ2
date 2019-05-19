// server.c 
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019

#include "log_writing.h"
#include <pthread.h>

pthread_mutex_t serverlog_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logBankOfficeOpen(fd, id, pthread_self());
    close(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}

void bankOfficeCloseLogWriting(int id)
{
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logBankOfficeClose(fd, id, pthread_self());
    close(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}

void accountCreationLogWriting(const bank_account_t *account, int id)
{
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logAccountCreation(fd, id, account);
    closeLog(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}

void requestSentLogWriting(const tlv_request_t *request, int id)
{
    int fd=openLog(USER_LOGFILE);
    logRequest(fd, id, request);
    closeLog(fd);
}

void requestReceivedLogWriting(const tlv_request_t *request, int id)
{
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logRequest(fd, id, request);
    closeLog(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}

void replySentLogWriting(const tlv_reply_t *reply, int id)
{
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logReply(fd, id, reply);
    closeLog(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}

void replyReceivedLogWriting(const tlv_reply_t *reply, int id)
{
    int fd=openLog(USER_LOGFILE);
    logReply(fd, id, reply);
    closeLog(fd);
}

void syncMechLogWriting(int id, sync_mech_op_t smo, sync_role_t role, int sid)
{
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logSyncMech(fd, id, smo, role, sid);
    close(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}

void syncMechSemLogWriting(int id, sync_mech_op_t smo, sync_role_t role, int sid, int val)
{
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logSyncMechSem(fd, id, smo, role, sid, val);
    close(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}

void delayLogWriting(int id, uint32_t delay_ms)
{
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logDelay(fd, id, delay_ms);
    close(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}

void syncDelayLogWriting(int id, int sid, uint32_t delay_ms)
{
    pthread_mutex_lock(&serverlog_mutex);
    int fd=openLog(SERVER_LOGFILE);
    logSyncDelay(fd, id, sid, delay_ms);
    close(fd);
    pthread_mutex_unlock(&serverlog_mutex);
}