// server.c 
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019
#include <pthread.h> 
#include <string.h>
#include "log_writing.h"
#include "utils.h"

int server_fifo_fd;
bool closed;

static bank_account_t accounts[MAX_BANK_ACCOUNTS];

void initializeAccountsArray()
{
    for (int i=0; i<MAX_BANK_ACCOUNTS; i++)
        accounts[i].account_id=1;
}

ret_code_t verifyCreateAccount(int id)
{
    if (accounts[id].account_id==1)
        return RC_ID_IN_USE;
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
    if (accounts[id_giver].account_id==1 || accounts[id_receiver].account_id==1)
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
    if (accounts[id_account].account_id==1)
        return RC_ID_NOT_FOUND;
    return RC_OK;
}

uint32_t consultBalance(int id_account)
{
    return accounts[id_account].balance;
}

bool verifyAuthenticity(int id_account, char* password)
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

bool verifyAuthorization(int id_account, int operation_code)
{
    if (( operation_code== 0 || operation_code==3) && id_account!=ADMIN_ACCOUNT_ID )
        return false;
    return true;
}

void closeBank(int server_fifo_fd)
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

int main(int argc, char* argv[])
{
    closed=false;
    initializeAccountsArray();
    int number_counters = argument_handler(argc, argv);

    int fd_dummy;
    createFifo(SERVER_FIFO_PATH);
    server_fifo_fd = openReadFifo(SERVER_FIFO_PATH, &fd_dummy);

    for (int i=0; i<number_counters; i++)
    {
        bankOfficeOpenLogWriting(i);
        //create threads
    }

    //ciclo (while !closed) para receber pedidos e colocar na fila de pedidos
    //as threads vao buscar as cenas a fila de pedidos e processam os pedidos

    //esperar que todas as thread terminem de processar todos os pedidos
    /*for (int i = 0; i < 2; i++) {
    pthread_join(tid[i], NULL);*/  


    closeUnlinkFifo(SERVER_FIFO_PATH, server_fifo_fd, fd_dummy);

    return 0;
}