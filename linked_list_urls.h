#ifndef LINKED_LIST_URLS_H
#define LINKED_LIST_URLS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct URLNode {
    struct URLNode* next;
    char* url;
};
typedef struct URLNode URLNode_t;

URLNode_t* createURLNode(char* url);

void pushURLList(URLNode_t** head, char* url);

char* popURLList(URLNode_t** head);

void printURLList(URLNode_t* head);

int getURLListLength(URLNode_t* head);

int findURLList(URLNode_t* head, char* url);
#endif