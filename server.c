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
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void initializeAccountsArray()
{
    for (int i=0; i<MAX_BANK_ACCOUNTS; i++)
        accounts[i].account_id=1;
}

ret_code_t check_account_creation(int id, int balance, char password[]) {
    if(accounts[id].account_id!=-1) {
        return RC_ID_IN_USE;
    }

    if (strlen(password)<MIN_PASSWORD_LEN || strlen(password)>MAX_PASSWORD_LEN) {
        return RC_OTHER;
    }

    if (balance < MIN_BALANCE || balance > MAX_BALANCE) {
        return RC_OTHER;
    }

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
    strcpy(account.hash, hash);

    accounts[id]=account;

    accountCreationLogWriting(&account, thread_id);
}

ret_code_t verifyTransfer(int id_giver, int id_receiver, int amount)
{
    if (accounts[id_giver].account_id==-1 || accounts[id_receiver].account_id==-1)
        return RC_ID_NOT_FOUND;
    if (id_giver == id_receiver)
        return RC_SAME_ID;
    if (accounts[id_giver].balance - amount < MIN_BALANCE)
        return RC_NO_FUNDS;
    if (accounts[id_receiver].balance + amount > MAX_BALANCE)
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

bool checkLogin(int id_account, char password)
{
    char salt[SALT_LEN+1];
    strcpy(salt, accounts[id_account].salt);

    char salt_plus_password[strlen(password)+SALT_LEN+1];
    strcpy(salt_plus_password, password);
    strcat(salt_plus_password, salt);

    char hash[HASH_LEN+1];
    produceSha(salt_plus_password, hash);
    
    if (hash == accounts[id_account].hash)
        return true;
    return false;
}

ret_code_t check_permissions(int id_account, int operation_code)
{
    if (( operation_code== 0 || operation_code==3) && id_account!=ADMIN_ACCOUNT_ID )
        return RC_OP_NALLOW;
    return RC_OK;
}

//completar mais tarde
void bank_shutdown(int server_fifo_fd)
{
    closed = true;
    //changing permission of the fifo
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
    createAccount(ADMIN_ACCOUNT_ID, 0, password, 0);

    return number_counters;
}

void initializeSems(int counter_number) {
    sem_init(&empty,0,counter_number);
    sem_init(&full,0,0);
    return;
}

void op_balance_handler(tlv_reply_t *reply, tlv_request_t request) {
    reply->value.header.account_id = request.value.header.account_id;
    reply->type = OP_BALANCE;
    reply->value.balance.balance = accounts[request.value.header.account_id].balance;
    reply->value.header.ret_code = RC_OK;
    //lenght calculations
    reply->length = sizeof(rep_header_t) + sizeof(rep_balance_t);
    return;
}

void op_transfer_handler(tlv_reply_t *reply, tlv_request_t request) {
    int sender = request.value.header.account_id;
    int receiver = request.value.transfer.account_id;
    int amount = request.value.transfer.amount;

    reply->value.header.account_id = request.value.header.account_id;
    reply->type = OP_TRANSFER;

    reply->value.header.ret_code = verifyTransfer(sender, receiver, amount);
    if(reply->value.header.ret_code!=RC_OK) return;
    else transfer(sender,receiver,amount);

    reply->value.transfer.balance = accounts[request.value.header.account_id].balance;
    //lenght calculations
    reply->length = sizeof(rep_header_t) + sizeof(rep_transfer_t);

    return;
}

// não confirmei assim beeeem mas acho que o reply n precisa
// de mais argumentos. confirmar melhor mais tarde
void op_create_account_handler(tlv_reply_t *reply, tlv_request_t request) {
    reply->value.header.account_id = request.value.header.account_id;
    reply->type = OP_CREATE_ACCOUNT;
    //os testes estão nesta ordem especifica devido à ordem pedida
    //no caso de houverem mais erros
    reply->value.header.ret_code = check_permissions(request.value.header.account_id)!=RC_OK);
    if(reply->value.header.ret_code!=RC_OK) return;

    int id = request.value.create.account_id;
    int balance = request.value.create.balance;
    char passw[MAX_PASSWORD_LEN] = request.value.create.password;

    reply->value.header.ret_code = check_account_creation(id, balance, passw);
    if(reply->value.header.ret_code!=RC_OK) return;

    createAccount(id, balance, passw, (int)pthread_self());

    return;
}

void sendReply(tlv_request_t request) {
    tlv_reply_t reply;

    reply.value.header.account_id = request.value.header.account_id;
    pid_t pid = request.value.header.pid;
    uint32_t account_id = request.value.header.account_id;

    //checking if account exists
    if(verifyAccountExistance(request.value.header.account_id)!=RC_OK) {
        reply.value.header.ret_code = RC_ID_NOT_FOUND;
    }
    //checking if password is correct
    else if(!checkLogin(request.value.header.account_id,request.value.header.password)) {
        reply.value.header.ret_code = RC_LOGIN_FAIL;
    }
    //making operation specific checks and operations
    else {
        switch(request.type) {
            case OP_BALANCE:
                op_balance_handler(&reply,request);
                break;
            case OP_TRANSFER:
                op_transfer_handler(&reply,request);
                break;
            case OP_CREATE_ACCOUNT:
                op_create_account_handler(&reply,request);
                break;
            case OP_SHUTDOWN: 
                //completar mais tarde(func n esta feita)
                break;
        }
    }
    //making reply fifo
    char reply_fifo_path[16];
    sprintf(reply_fifo_path, "%s%0*d", USER_FIFO_PATH_PREFIX, WIDTH_ID, request.value.header.pid);
    int reply_fifo_fd = openReadFifo(reply_fifo_path);
    //valta verificar erros no write (perror???)
    write(reply_fifo_fd, &reply, sizeof(reply));

    close(reply_fifo_fd);
}

void counter() {
    //falta o sem_getvalue que pelos vistos é preciso
    //para os logs (???)
    //Dúvida: basta dar o lock do mutex quando se vai retirar
    //algo da queue ou também enquanto se envia a resposta?
    //primeiro apenas, digo eu
    while(1) {
        sem_wait(&full);
        pthread_mutex_lock(&mutex);

        tlv_request_t request;
        queue_remove(&request);
        //sendReply()


        pthread_mutex_unlock(&mutex);
        sem_post(&empty);
        
    }
    return;
}

void create_counters(counter_number) {
    for(int i=0; i<counter_number; i++) {
        pthread_create(&counters[i], NULL, counter, NULL);
    }
    return;
}

int main(int argc, char* argv[])
{
    closed=false;
    initializeAccountsArray();
    int counter_number = argument_handler(argc, argv);

    createFifo(SERVER_FIFO_PATH);
    server_fifo_fd = openReadFifo(SERVER_FIFO_PATH);

    create_counters(counter_number);



    //ciclo (while !closed) para receber pedidos e colocar na fila de pedidos
    //as threads vao buscar as cenas a fila de pedidos e processam os pedidos

    //esperar que todas as thread terminem de processar todos os pedidos
    /*for (int i = 0; i < 2; i++) {
    pthread_join(tid[i], NULL);*/  


    closeUnlinkFifo(SERVER_FIFO_PATH, server_fifo_fd);

    return 0;
}