#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "types.h"

#define MAX_REQUESTS 100

tlv_request_t request_queue[MAX_REQUESTS];
unsigned size = 0;
unsigned rear = MAX_REQUESTS - 1;
unsigned front = 0;

int isFull() {
    return (size==MAX_REQUESTS);
}

int isEmpty() {
    return (size==0);
}

int queue_insert(tlv_request_t request) {
    if(isFull()) return -1;
    rear = (rear + 1) % MAX_REQUESTS;
    size++;
    request_queue[rear] = request;
    return 0;
}

int queue_remove(tlv_request_t *request) {
    if(isEmpty()) return -1;
    *request = request_queue[front];
    front = (front + 1) % MAX_REQUESTS;
    size--;
    return 0;
}