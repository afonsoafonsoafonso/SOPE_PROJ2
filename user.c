// user.c 
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019
 
#include <pthread.h> 
#include <string.h>
#include "log_writing.h"
#include "utils.h"
#include <signal.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>
#include <stdio.h> 
static tlv_request_t request;
bool timeout = false;

void sigalarm_handler(int signo) {  
    printf("In SIGALRM handler ...\n");
    timeout=true;
    //print_logf
} 

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
        printf("Balances must have a value between %ld and %ld.\n", MIN_BALANCE, MAX_BALANCE);
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
        printf("Amounts transfered must have a value between 1 and %ld.\n", MAX_BALANCE);
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
    strcpy(request.value.header.password, password); 
    //operation_code
    int operation_code=atoi(argv[4]);
    if (operation_code<0 || operation_code>3)
    {
        printf("Operation code must be between 0 and 3.\n");
        exit(1);
    }
    request.type = operation_code;
    //pid
    request.value.header.pid = getpid();
    //account_id
    if (atoi(argv[1])<0 || atoi(argv[1]) >=MAX_BANK_ACCOUNTS)
    {
        printf("Accounts numbers are all between 0 and %d.\n", MAX_BANK_ACCOUNTS);
        exit(1);
    }
    request.value.header.account_id = atoi(argv[1]);
    //op_delay_ms
    if(atoi(argv[3])<0 || atoi(argv[3])>MAX_OP_DELAY_MS) {
        printf("Op delay must be between 0 and %d.\n", MAX_OP_DELAY_MS);
    }
    request.value.header.op_delay_ms = atoi(argv[3]);

    if (operation_code == 0) {
        request.value.create=create_account_argument_handler(argv[5], strlen(argv[5]));
        request.length = sizeof(req_header_t) + sizeof(req_create_account_t);
    }
    else if (operation_code == 3) {
        request.value.transfer=transfer_argument_handler(argv[5], strlen(argv[5]));
        request.length = sizeof(req_header_t) + sizeof(req_create_account_t);
    }
    else if (strlen(argv[5])!=0)
        printf("This operation does not take any kind of arguments. Arguments inserted will be ignored.\n");
}

int sendRequest(tlv_request_t request, int request_fifo_fd) {
    if(write(request_fifo_fd, &request, sizeof(tlv_request_t))==-1) {
        return -1;
    }
    return 0;
}

//falta fazer contagem dos 30 segundos. usar um novo thread para isto???
//L- acho que faz mais sentido por o user a enviar u sinal para este. assim aqui podes ter um sleep
//   (se devolver 0 quer dizer que passaram os 30 seg, se devolver outra coisa quer dizer que recebeu o sinal e demorou menos de 30seg)
ret_code_t receiveReply(int reply_fifo_fd, tlv_reply_t *reply) {
    struct sigaction sigalarm;
    sigalarm.sa_handler=sigalarm_handler;
    sigalarm.sa_flags=SA_RESTART;
    sigemptyset(&sigalarm.sa_mask);
    if (sigaction(SIGALRM,&sigalarm,NULL) < 0)   {
        fprintf(stderr,"Unable to install SIGALRM handler\n");     
        exit(1);   }  
        alarm(30); 
    while(!timeout) {
        if(read(reply_fifo_fd, reply, sizeof(tlv_reply_t))==sizeof(tlv_reply_t)) {//falta o log
            //fazer o que há para fazer(caso haja algo mais que dar return ao codigo)
            alarm(0);//para os alarmes pendentes
            return reply->value.header.ret_code;
        }    
    }
    
    return 0;//reply->value.header.ret_code;
}

int main(int argc, char* argv[])
{
    argument_handler(argc, argv);
    printf("teste 1\n");
    int request_fifo_fd = openWriteFifo(SERVER_FIFO_PATH);
    printf("teste 2\n");
    char reply_fifo_path[16];
    printf("teste 3\n");
    sprintf(reply_fifo_path, "%s%0*d", USER_FIFO_PATH_PREFIX, 5, getpid());
    printf("teste 4\n");
    createFifo(reply_fifo_path);
    printf("teste 5\n");
    int reply_fifo_fd = openReadFifo(reply_fifo_path);
    printf("teste 6\n");
    sendRequest(request, request_fifo_fd);
    printf("teste 7\n");
    close(request_fifo_fd);
    printf("teste 8\n");
    closeUnlinkFifo(reply_fifo_path, reply_fifo_fd);
    printf("teste 9\n");
    return 0;
}