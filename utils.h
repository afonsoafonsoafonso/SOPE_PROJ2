// server.c 
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019

void produceSha(const char* toEncrypt, char* encrypted);

void produceSalt(char* salt);

int createFifo(char* fifo_name);

int openReadFifo(char* fifo_name);

int openWriteFifo(char* fifo_name);

void closeUnlinkFifo(char* fifo_name, int fd);