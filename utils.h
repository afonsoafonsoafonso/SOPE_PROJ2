void produceSha(const char* toEncrypt, char* encrypted);

void produceSalt(char* salt);

int createFifo(char* fifo_name);

int openReadFifo(char* fifo_name, int * fd_dummy);

int openWriteFifo(char* fifo_name);

void closeUnlinkFifo(char* fifo_name, int fd, int fd_dummy);