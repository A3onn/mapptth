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

void pushURLStack(URLNode_t** head, char* url);

char* popURLStack(URLNode_t** head);

void printURLStack(URLNode_t* head);

int getURLStackLength(URLNode_t* head);

int isURLStackEmpty(URLNode_t* head);

int findURLStack(URLNode_t* head, char* url);
#endif