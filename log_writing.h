#include <stdlib.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sope.h"

void bankOfficeOpenLogWriting(int id);

void bankOfficeCloseLogWriting(int id);

void accountCreationLogWriting(const bank_account_t *account, int id);