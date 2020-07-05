#include "linked_list_urls.h"

int pushURLList(URLNode_t** head, char* url) {
    URLNode_t* newNode = (URLNode_t*) malloc(sizeof (URLNode_t));
    newNode->next = *head;
    newNode->url = url;
    *head = newNode;
    return 1;
}

char* popURLList(URLNode_t** head) {
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