#include "queue_urls.h"

void pushURLQueue(URLNode_t** head, char* url) {
    URLNode_t* newNode = (URLNode_t*) malloc(sizeof(URLNode_t));
    newNode->next = *head;
    newNode->url = url;
    *head = newNode;
}

char* popURLQueue(URLNode_t** head) {
    URLNode_t* tmp = *head;
    if(tmp == NULL) {
        return NULL;
    }
    *head = tmp->next;
    char* res = tmp->url;
    free(tmp);
    tmp = NULL;
    return res;
}

int isURLQueueEmpty(URLNode_t* head) {
    return head == 0;
}

int getURLQueueLength(URLNode_t* head) {
    URLNode_t* tmp = head;
    if(tmp == NULL) {
        return 0;
    }

    int res = 0;
    for(; tmp != NULL; tmp = tmp->next) {
        res++;
    }

    return res;
}

int findURLQueue(URLNode_t* head, char* url) {
    URLNode_t* tmp = head;
    if(tmp == NULL) {
        return 0;
    }

    for(; tmp != NULL; tmp = tmp->next) {
        if(strcmp(tmp->url, url) == 0) {
            return 1;
        }
    }

    return 0;
}

void printURLQueue(URLNode_t* head) {
    if(head == NULL) {
        puts("<empty>");
        return;
    }
    URLNode_t* tmp = head;

    while(tmp != NULL) {
        printf("%s - ", tmp->url);
        tmp = tmp->next;
    }
    printf("\n");
}