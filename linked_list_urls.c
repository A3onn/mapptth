#include "linked_list_urls.h"

void pushURLList(URLNode_t** head, char* url) {
    URLNode_t* newNode = (URLNode_t*) malloc(sizeof (URLNode_t));
    newNode->next = *head;
    newNode->url = url;
    *head = newNode;
}

char* popURLList(URLNode_t** head) {
    URLNode_t* tmp = *head;
    if(tmp == NULL) {
        return NULL;
    }
    *head = tmp->next;
    char* res = (char*) malloc(sizeof (char) * strlen(tmp->url)+1);
    strncpy(res, tmp->url, strlen(tmp->url)+1);
    free(tmp);
    tmp = NULL;
    return res;
}

URLNode_t* createURLNode(char* url) {
    URLNode_t* newNode = (URLNode_t*) malloc(sizeof (URLNode_t));
    newNode->url = url;
    newNode->next = NULL;
    return newNode;
}

int getURLListLength(URLNode_t* head) {
    URLNode_t* tmp = head;
    if(tmp == NULL) {
        return 0;
    }

    int res = 0;
    for(; tmp != NULL; tmp = tmp->next){res++;}

    return res;
}

int findURLList(URLNode_t* head, char* url) {
    URLNode_t* tmp = head;
    if(tmp == NULL) {
        return 0;
    }

    for(; tmp != NULL; tmp = tmp->next){
        if(strcmp(tmp->url, url) == 0) {
            return 1;
        }
    }

    return 0;
}

void printURLList(URLNode_t* head) {
    URLNode_t* tmp = head;
    if(tmp->next == NULL) {
        puts("<empty>");
        return;
    }

    while(tmp != NULL) {
        printf("%s - ", tmp->url);
        tmp = tmp->next;
    }
    printf("\n");
}