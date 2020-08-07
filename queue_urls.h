#ifndef LINKED_LIST_URLS_H
#define LINKED_LIST_URLS_H

// FIFO

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct URLNode {
    struct URLNode* next;
    char* url;
};
typedef struct URLNode URLNode_t;

void pushURLQueue(URLNode_t** head, char* url);

char* popURLQueue(URLNode_t** head);

void printURLQueue(URLNode_t* head);

int getURLQueueLength(URLNode_t* head);

int isURLQueueEmpty(URLNode_t* head);

int findURLQueue(URLNode_t* head, char* url);
#endif