#include "linked_list_urls.h"

int pushURLList(URLNode_t** head, const char* url) {
    URLNode_t* newNode = (URLNode_t*) malloc(sizeof (URLNode_t));
    newNode->next = *head;
    newNode->url = curl_url();
    if(!newNode->url) {
        return 0;
    }
    CURLUcode i = curl_url_set(newNode->url, CURLUPART_URL, url, 0);
    if(i != CURLUE_OK) {
        return 0;
    }
    *head = newNode;
    return 1;
}

CURLU* popURLList(URLNode_t** head) {
    URLNode_t* tmp = *head;
    if(tmp == NULL) {
        return NULL;
    }
    *head = tmp->next;
    CURLU * res = tmp->url;
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

    char* host;
    for(; tmp != NULL; tmp = tmp->next){
        curl_url_get(tmp->url, CURLUPART_URL, &host, 0);
        if(strcmp(host, url) == 0) {
            free(host);
            return 1;
        }
        free(host);
    }

    return 0;
}

void printURLList(URLNode_t* head) {
    if(head == NULL) {
        puts("<empty>");
        return;
    }
    URLNode_t* tmp = head;

    char* url;
    while(tmp != NULL) {
        curl_url_get(tmp->url, CURLUPART_URL, &url, 0);
        printf("%s - ", url);
        tmp = tmp->next;
    }
    printf("\n");
}