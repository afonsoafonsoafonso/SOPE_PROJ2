// server.c 
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019
#include <pthread.h> 
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include "log_writing.h"
#include "utils.h"
#include "queue.h"

int server_fifo_fd;
bool closed;

sem_t full, empty;
static bank_account_t accounts[MAX_BANK_ACCOUNTS];
static pthread_t counters[MAX_BANK_OFFICES];
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
extern tlv_request_t request_queue[MAX_REQUESTS];
//static pthread_mutex_t mutexes[MAX_BANK_ACCOUNTS];//={ [0 ... MAX_BANK_ACCOUNTS] = PTHREAD_MUTEX_INITIALIZER };;


void initializeAccountsArray()
{
    for (int i=0; i<MAX_BANK_ACCOUNTS; i++){
        accounts[i].account_id=-1;
        //accounts[i].mutex = mutexes[i];
    }
}

ret_code_t check_account_creation(int id, int balance, char password[]) {
    if(accounts[id].account_id!=-1) {
        return RC_ID_IN_USE;
    }

    if (strlen(password)<MIN_PASSWORD_LEN || strlen(password)>MAX_PASSWORD_LEN) {
        return RC_OTHER;
    }//-> desnecessário. já é verificado no programa user

    if (balance < MIN_BALANCE || balance > MAX_BALANCE) {
        return RC_OTHER;
    } //-> desnecessário. já é verificado no programa user

    return RC_OK;
}

void createAccount(int id, int balance, char* password, int thread_id)
{
    bank_account_t account;
    account.account_id=id;
    account.balance=balance;

    char salt[SALT_LEN+1];
    produceSalt(salt);
    strcpy(account.salt, salt);

    char salt_plus_password[strlen(password)+SALT_LEN+1];
    strcpy(salt_plus_password, password);
    strcat(salt_plus_password, salt);
    char hash[HASH_LEN+1];
    
    produceSha(salt_plus_password, hash);
    
    strncpy(account.hash, hash,HASH_LEN+1);
    accounts[id]=account;

    accountCreationLogWriting(&account, thread_id);

}

ret_code_t verifyTransfer(int id_giver, int id_receiver, int amount)
{
    if (accounts[id_giver].account_id==-1 || accounts[id_receiver].account_id==-1)
        return RC_ID_NOT_FOUND;
    if (id_giver == id_receiver)
        return RC_SAME_ID;
    if ((int)accounts[id_giver].balance - (int)amount < (int)MIN_BALANCE)
        return RC_NO_FUNDS;
    if ((int)accounts[id_receiver].balance + (int)amount > (int)MAX_BALANCE)
        return RC_TOO_HIGH;
    return RC_OK;
}

void transfer(int id_giver, int id_receiver, int amount)
{
    accounts[id_giver].balance-=amount;
    accounts[id_receiver].balance+=amount;
}

ret_code_t verifyAccountExistance(int id_account) //used in consultBalance
{
    if (accounts[id_account].account_id==-1)
        return RC_ID_NOT_FOUND;
    return RC_OK;
}

uint32_t consultBalance(int id_account)
{
    return accounts[id_account].balance;
}

bool checkLogin(int id_account, char* password)
{
    char salt[SALT_LEN+1];
    strcpy(salt, accounts[id_account].salt);

    char salt_plus_password[strlen(password)+SALT_LEN+1];
    strcpy(salt_plus_password, password);
    strcat(salt_plus_password, salt);

    char hash[HASH_LEN+1];
    produceSha(salt_plus_password, hash);
    if (strcmp(hash, accounts[id_account].hash)==0)
        return true;
    return false;
}

bool check_permissions(int id_account, int operation_code)
{
    if (( operation_code== OP_CREATE_ACCOUNT || operation_code==OP_SHUTDOWN) && id_account!=ADMIN_ACCOUNT_ID )
        return false;
    return true;
}

//completar mais tarde
void bank_shutdown()
{
    closed = true;
    //changing permission of the fifo
    chmod(SERVER_FIFO_PATH, 0444);
}

int argument_handler(int argc, char* argv[])
{
    if (argc!=3)
    {
        printf("Number of arguments unexpected.\nPlease use the following syntax: ./server <number_eletronic_counters> \"<password>\"\n");
        exit(1);
    }
    int number_counters = atoi(argv[1]);
    if (number_counters<0 || number_counters>MAX_BANK_OFFICES)
    {
        printf("Number of eletronic counters must be less or equal to %d.\n", MAX_BANK_OFFICES);
        exit(1);
    }
    char password[MAX_PASSWORD_LEN+1];
    strcpy(password,argv[2]);
    if (strlen(password)<MIN_PASSWORD_LEN || strlen(password)>MAX_PASSWORD_LEN)
    {
        printf("Password should have a length between %d and %d.\n", MIN_PASSWORD_LEN, MAX_PASSWORD_LEN);
        exit(2);
    } 
    //inicializing administrator account
    //pthread_mutex_lock(&(accounts[ADMIN_ACCOUNT_ID].mutex));
    //syncMechLogWriting(0, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, ADMIN_ACCOUNT_ID);

    usleep(0*1000);
    syncDelayLogWriting(0, ADMIN_ACCOUNT_ID, 0);
    createAccount(ADMIN_ACCOUNT_ID, 0, password, 0);

    //pthread_mutex_unlock(&(accounts[ADMIN_ACCOUNT_ID].mutex));
    //syncMechLogWriting(0, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, ADMIN_ACCOUNT_ID);
    
    return number_counters;
}

void initializeSems(int counter_number)
{
    //int sem_value;
    //sem_getvalue(&empty, &sem_value);
    syncMechSemLogWriting(0, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, 0, counter_number);
    sem_init(&empty,0,counter_number);

    //sem_getvalue(&full, &sem_value);
    syncMechSemLogWriting(0, SYNC_OP_SEM_INIT, SYNC_ROLE_PRODUCER, 0, 0);
    sem_init(&full,0,0);
}

void op_balance_handler(tlv_reply_t *reply, int counter_id, tlv_request_t request)
{
    int account_id=reply->value.header.account_id;
    printf("Antes do mutex\n");
    //pthread_mutex_lock(&(accounts[account_id].mutex));
    printf("Depois do mutex\n");
    syncMechLogWriting(counter_id, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, account_id);

    usleep(request.value.header.op_delay_ms*1000);
    syncDelayLogWriting(counter_id, account_id, request.value.header.op_delay_ms);

    reply->value.header.ret_code = verifyAccountExistance(account_id);
    if (reply->value.header.ret_code == RC_OK)
        reply->value.balance.balance = consultBalance(account_id);

    //pthread_mutex_unlock(&(accounts[account_id].mutex));
    syncMechLogWriting(counter_id, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, account_id);
}

void op_transfer_handler(tlv_reply_t *reply, tlv_request_t request, int counter_id)
{
    int sender = request.value.header.account_id;
    int receiver = request.value.transfer.account_id;
    int amount = request.value.transfer.amount;

    //pthread_mutex_lock(&(accounts[sender].mutex));
    //syncMechLogWriting(counter_id, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, sender);
    //pthread_mutex_lock(&(accounts[receiver].mutex));
    //syncMechLogWriting(counter_id, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, receiver);

    usleep(request.value.header.op_delay_ms*1000);
    syncDelayLogWriting(counter_id, sender, request.value.header.op_delay_ms);

    reply->value.header.ret_code = verifyTransfer(sender, receiver, amount);
    if(reply->value.header.ret_code==RC_OK)
        transfer(sender,receiver,amount);
    reply->value.transfer.balance = accounts[sender].balance;

    //pthread_mutex_lock(&(accounts[receiver].mutex));
    //syncMechLogWriting(counter_id, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, receiver);
    //pthread_mutex_unlock(&(accounts[sender].mutex));
    //syncMechLogWriting(counter_id, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, sender);
}

// não confirmei assim beeeem mas acho que o reply n precisa
// de mais argumentos. confirmar melhor mais tarde
void op_create_account_handler(tlv_reply_t *reply, tlv_request_t request, int counter_id)
{
    int account_id=request.value.create.account_id;
    int balance = request.value.create.balance;
    char passw[MAX_PASSWORD_LEN];
    strcpy(passw, request.value.create.password);

    //pthread_mutex_lock(&(accounts[account_id].mutex));
    //syncMechLogWriting(counter_id, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_ACCOUNT, account_id);

    usleep(request.value.header.op_delay_ms*1000);
    syncDelayLogWriting(counter_id, account_id, request.value.header.op_delay_ms);

    reply->value.header.ret_code = check_account_creation(account_id, balance, passw);
    if (reply->value.header.ret_code!=RC_OK)
        return;
    createAccount(account_id, balance, passw, (int)pthread_self());

    //pthread_mutex_unlock(&(accounts[account_id].mutex));
    syncMechLogWriting(counter_id, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_ACCOUNT, account_id);
}

void op_close_bank_handler(tlv_reply_t *reply, int counter_id , tlv_request_t request)
{
    reply->value.header.ret_code=RC_OK;
    reply->value.shutdown.active_offices=0; // não sei que valor se poe aqui

    usleep(request.value.header.op_delay_ms*1000);
    delayLogWriting(counter_id, request.value.header.op_delay_ms);

    bank_shutdown();
}

void fillReply(tlv_reply_t *reply, tlv_request_t request)
{
    reply->value.header.account_id = request.value.header.account_id;
    reply->type= request.type;
    switch(request.type) {
        case OP_BALANCE:
            reply->length = sizeof(rep_header_t) + sizeof(rep_balance_t);
            break;
        case OP_TRANSFER:
            reply->length = sizeof(rep_header_t) + sizeof(rep_transfer_t);
            break;
        case OP_CREATE_ACCOUNT:
            reply->length = sizeof(rep_header_t) + sizeof(req_create_account_t);
            break;
        case OP_SHUTDOWN: 
            reply->length = sizeof(rep_header_t) + sizeof(rep_shutdown_t);
            break;
        default:
            break;
    }
}

void requestHandler(tlv_request_t request, int counter_id) {
    tlv_reply_t reply;
    fillReply(&reply, request);

    int account_id=request.value.header.account_id;
    //checking if password is correct
    if(!checkLogin(account_id,request.value.header.password))
        reply.value.header.ret_code = RC_LOGIN_FAIL;

    //checking permissions
    else if (!check_permissions(account_id, request.type))
        reply.value.header.ret_code = RC_OP_NALLOW;

    //making operation specific checks and operations
    else {
        switch(request.type) {
            case OP_BALANCE:
                printf("Chega ao balance handler\n");
                op_balance_handler(&reply, counter_id, request);
                break;
            case OP_TRANSFER:
                op_transfer_handler(&reply,request, counter_id);
                break;
            case OP_CREATE_ACCOUNT:
                op_create_account_handler(&reply,request, counter_id);
                break;
            case OP_SHUTDOWN: 
                op_close_bank_handler(&reply, counter_id, request);
                break;
            default:
                break;
        }
    }
    //making reply fifo
    char reply_fifo_path[18];
    sprintf(reply_fifo_path, "%s%0*d", USER_FIFO_PATH_PREFIX, WIDTH_ID, request.value.header.pid);

    int reply_fifo_fd = openWriteFifo(reply_fifo_path);
    //valta verificar erros no write (perror???)
    write(reply_fifo_fd, &reply, sizeof(reply));
    replySentLogWriting(&reply, counter_id);
    close(reply_fifo_fd);
}

void *counter(void *threadnum) {
    //falta o sem_getvalue que pelos vistos é preciso para os logs (???)
    int counter_id=*(int *) threadnum;
    bankOfficeOpenLogWriting(counter_id);
    int sem_value;
    sem_getvalue(&full, &sem_value);
    while(!closed && isEmpty(request_queue)) {
        sem_getvalue(&full, &sem_value);
        syncMechSemLogWriting(counter_id, SYNC_OP_SEM_WAIT, SYNC_ROLE_PRODUCER, 0, sem_value);
        sem_wait(&full);
        //pthread_mutex_lock(&queue_mutex);
        syncMechLogWriting(counter_id, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_CONSUMER, 0);
        tlv_request_t request;
        //lock aqui apenas imediatamente antes de retirar da queue?
        pthread_mutex_lock(&queue_mutex);
        queue_remove(&request);
        pthread_mutex_unlock(&queue_mutex);

        requestReceivedLogWriting(&request, counter_id);
        //pthread_mutex_unlock(&queue_mutex);
        syncMechLogWriting(counter_id, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_CONSUMER, request.value.header.account_id);
        requestHandler(request, counter_id);
        sem_post(&empty);
        sem_getvalue(&empty, &sem_value);
        syncMechSemLogWriting(counter_id, SYNC_OP_SEM_POST, SYNC_ROLE_PRODUCER, 0, sem_value);
    }
    bankOfficeCloseLogWriting(counter_id);
    return NULL;
}

void create_counters(int counter_number, int aux[]) {
    for(int i=0; i<counter_number; i++) {
        aux[i]=i+1;
        pthread_create(&counters[i], NULL, counter, (void *)&aux[i]);
    }
    return;
}

int main(int argc, char* argv[])
{
    close(open(SERVER_LOGFILE,O_CREAT|O_WRONLY|O_TRUNC,0666));//cleans log
    close(open(USER_LOGFILE,O_CREAT|O_WRONLY|O_TRUNC,0666));//cleans log
    closed=false;
    initializeAccountsArray();

    int counter_number = argument_handler(argc, argv);
    initializeSems(counter_number);

    int aux[counter_number];

    createFifo(SERVER_FIFO_PATH);
   
    server_fifo_fd = openReadFifo(SERVER_FIFO_PATH);

    create_counters(counter_number, aux);

    tlv_request_t request;
    int sem_value;

    while(!closed)
    {
        if(read(server_fifo_fd, &request, sizeof(tlv_request_t))==sizeof(tlv_request_t)){
            printf("teste 7\n");
            sem_getvalue(&empty,&sem_value);
            logSyncMechSem(0, MAIN_THREAD_ID, SYNC_OP_SEM_WAIT, SYNC_ROLE_PRODUCER, request.value.header.pid, sem_value);
            sem_wait(&empty);

            pthread_mutex_lock(&queue_mutex);
            syncMechLogWriting(0, SYNC_OP_MUTEX_LOCK, SYNC_ROLE_PRODUCER, 0);

            queue_insert(request);
            requestReceivedLogWriting(&request, 0);

            pthread_mutex_unlock(&queue_mutex);
            syncMechLogWriting(0, SYNC_OP_MUTEX_UNLOCK, SYNC_ROLE_PRODUCER, request.value.header.account_id);

            sem_post(&full);
            sem_getvalue(&full, &sem_value);
            logSyncMechSem(0, MAIN_THREAD_ID, SYNC_OP_SEM_POST, SYNC_ROLE_PRODUCER, request.value.header.pid, sem_value);
        }
    }

    //esperar que todas as thread terminem de processar todos os pedidos
    for (int i = 0; i < 2; i++)
        pthread_join(counters[i], NULL);

    closeUnlinkFifo(SERVER_FIFO_PATH, server_fifo_fd);

    return 0;
}