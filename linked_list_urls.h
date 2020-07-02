#ifndef LINKED_LIST_URLS_H
#define LINKED_LIST_URLS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#if !CURL_AT_LEAST_VERSION(7, 62, 0)
#error "this example requires curl 7.62.0 or later"
#endif

struct URLNode {
    struct URLNode* next;
    CURLU* url;
};
typedef struct URLNode URLNode_t;

int pushURLList(URLNode_t** head, const char* url);

CURLU* popURLList(URLNode_t** head);

void printURLList(URLNode_t* head);

int getURLListLength(URLNode_t* head);

int findURLList(URLNode_t* head, char* url);
#endif