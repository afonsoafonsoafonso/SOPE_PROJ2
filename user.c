// user.c 
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019
 
#include <pthread.h> 
#include <string.h>
#include "log_writing.h"

static req_value_t req_value;

req_create_account_t create_account_argument_handler(char* args, int args_size)
{
    
    req_create_account_t create;

    //taking the arguments from the string
    char *aux = strtok (args," ");
    int failure=0;

    if (aux!=NULL)
    {
        create.account_id=atoi(aux);

        aux=strtok(NULL, " ");
        if (aux!=NULL)
        {
            create.balance = atoi(aux);

            aux=strtok(NULL, " ");
            if (aux!=NULL)
                strcpy(create.password,aux);
            else
                failure=1;
        }
        else
            failure=1;
    }
    else
        failure=1;
    aux=strtok(NULL, " ");
    if (aux!=NULL)
        failure=1;
    if (failure)
    {
        printf("This operation expects 3 arguments.\n");
        exit(1);
    }

    //checking the arguments values
    if (create.account_id<1 || create.account_id>=MAX_BANK_ACCOUNTS)
    {
        printf("Accounts numbers are all between 1 and %d.\n", MAX_BANK_ACCOUNTS);
        exit(1);
    }
    if (create.balance<MIN_BALANCE || create.balance>MAX_BALANCE)
    {
        printf("Balances must have a value between %d and %d.\n", MIN_BALANCE, MAX_BALANCE);
        exit(1);
    }
    if (strlen(create.password)<MIN_PASSWORD_LEN || strlen(create.password)>MAX_PASSWORD_LEN)
    {
        printf("Password should have a length between %d and %d.\n", MIN_PASSWORD_LEN, MAX_PASSWORD_LEN);
        exit(1);
    } 

    return create;
}

req_transfer_t transfer_argument_handler(char* args, int args_size)
{
    req_transfer_t transfer;

    //taking the arguments from the string
    char *aux = strtok (args," ");
    int failure=0;

    if (aux!=NULL)
    {
        transfer.account_id=atoi(aux);
        printf("%d\n", transfer.account_id);

        aux=strtok(NULL, " ");
        if (aux!=NULL)
            transfer.amount=atoi(aux); 
        else
            failure=1;
        printf("%d\n", transfer.amount);  
    }
    else
        failure=1;
    aux=strtok(NULL, " ");
    if (aux!=NULL)
        failure=1;
    if (failure)
    {
        printf("This operation expects 2 arguments.\n");
        exit(1);
    }

    //checking the arguments values
    if (transfer.account_id<1 || transfer.account_id>=MAX_BANK_ACCOUNTS)
    {
        printf("Accounts numbers are all between 1 and %d.\n", MAX_BANK_ACCOUNTS);
        exit(1);
    }
    if (transfer.amount<=1 || transfer.amount>MAX_BALANCE)
    {
        printf("Amounts transfered must have a value between 1 and %d.\n", MAX_BALANCE);
        exit(1);
    }

    return transfer;
}

void argument_handler(int argc, char* argv[])
{
    if (argc!=6)
    {
        printf("Number of arguments unexpected.\nPlease use the following syntax: ./user <account_id> \"<password>\" <delay in ms> <operation_code> \"<list of arguments>\"\n");
        exit(1);
    }
    //password
    char password[MAX_PASSWORD_LEN+1];
    strcpy(password,argv[2]);
    if (strlen(password)<MIN_PASSWORD_LEN || strlen(password)>MAX_PASSWORD_LEN)
    {
        printf("Password should have a length between %d and %d.\n", MIN_PASSWORD_LEN, MAX_PASSWORD_LEN);
        exit(2);
    } 
    //operation_code
    int operation_code=atoi(argv[4]);
    if (operation_code<0 || operation_code>3)
    {
        printf("Operation code must be between 0 and 3.\n");
        exit(1);
    }

    req_header_t header;
    header.pid=getpid();
    header.account_id=atoi(argv[1]);
    if (header.account_id<0 || header.account_id>=MAX_BANK_ACCOUNTS)
    {
        printf("Accounts numbers are all between 0 and %d.\n", MAX_BANK_ACCOUNTS);
        exit(1);
    }

    strcpy(header.password, password);
    header.op_delay_ms=atoi(argv[3]);

    req_value.header=header;

    if (operation_code == 0)
        req_value.create=create_account_argument_handler(argv[5], strlen(argv[5]));
    else if (operation_code == 3)
        req_value.transfer=transfer_argument_handler(argv[5], strlen(argv[5]));
    else if (strlen(argv[5])!=0)
        printf("This operation does not take any kind of arguments. Arguments inserted will be ignored.\n");

}

int main(int argc, char* argv[])
{
    argument_handler(argc, argv);
    return 0;
}