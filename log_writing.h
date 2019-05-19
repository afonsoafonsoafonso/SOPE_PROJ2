// server.c 
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019

#include <stdlib.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sope.h"

void bankOfficeOpenLogWriting(int id);

void bankOfficeCloseLogWriting(int id);

void accountCreationLogWriting(const bank_account_t *account, int id);

void requestSentLogWriting(const tlv_request_t *request, int id);

void requestReceivedLogWriting(const tlv_request_t *request, int id);

void replySentLogWriting(const tlv_reply_t *reply, int id);

void replyReceivedLogWriting(const tlv_reply_t *reply, int id);

void syncMechLogWriting(int id, sync_mech_op_t smo, sync_role_t role, int sid);

void syncMechSemLogWriting(int id, sync_mech_op_t smo, sync_role_t role, int sid, int val);

void delayLogWriting(int id, uint32_t delay_ms);

void syncDelayLogWriting(int id, int sid, uint32_t delay_ms);
