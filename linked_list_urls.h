#ifndef LINKED_LIST_URLS_H
#define LINKED_LIST_URLS_H

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct URLNode {
    struct URLNode* next;
    char* url;
};
typedef struct URLNode URLNode_t;

void pushURLList(URLNode_t** head, char* url);

char* popURLList(URLNode_t** head);

void printURLList(URLNode_t* head);

int getURLListLength(URLNode_t* head);

int isURLListEmpty(URLNode_t* head);

int findURLList(URLNode_t* head, char* url);
#endif