#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "types.h"

extern tlv_request_t* request_queue;
extern unsigned size;
extern unsigned rear;
extern unsigned front;

int isFull();

int isEmpty();

int queue_insert(tlv_request_t request);

int queue_remove(tlv_request_t* request);