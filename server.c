// server.c 
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019
#include <pthread.h> 
#include <string.h>
#include "log_writing.h"

static bank_account_t administrator;

int argument_handler(int argc, char* argv[])
{
    if (argc!=3)
    {
        printf("Number of arguments unexpected.\nPlease use the following syntax: ./server number_eletronic_counters \"password\"\n");
        exit(1);
    }
    int number_counters = atoi(argv[1]);
    char password[MAX_PASSWORD_LEN];
    strcpy(password,argv[2]);
    if (strlen(password)<MIN_PASSWORD_LEN || strlen(password)>MAX_PASSWORD_LEN)
    {
        printf("Password should have a length between %d and %d.\n", MIN_PASSWORD_LEN, MAX_PASSWORD_LEN);
        exit(2);
    } 
    //inicializing administrator account
    administrator.account_id=ADMIN_ACCOUNT_ID;
    administrator.balance=0;
    strcpy(administrator.hash,"hash");     //////////////////////////////////////////////////////////////////to complete (auxiliary function)
    strcpy(administrator.salt,"salt");     //////////////////////////////////////////////////////////////////to complete (auxiliary function)
    accountCreationHandler(&administrator);

    return number_counters;
}

int main(int argc, char* argv[])
{
    int number_counters = argument_handler(argc, argv);
    return 0;
}