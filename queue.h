// queue.h
// Developed by: 
// João Araujo - 201705577
// Leonor Sousa - 201705377
// Afonso Mendonça - 201706708
// SOPE - MIEIC2 - 2018/2019

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "types.h"

extern unsigned size;
extern unsigned rear;
extern unsigned front;

int isFull();

int isEmpty();

int queue_insert(tlv_request_t request);

int queue_remove(tlv_request_t* request);