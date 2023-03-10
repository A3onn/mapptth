#include "stack_urls.h"

void stack_url_push(URLNode_t** head, char* url) {
    URLNode_t* new_node = (URLNode_t*) malloc(sizeof(URLNode_t));
    new_node->next = *head;
    new_node->url = url;
    *head = new_node;
}

char* stack_url_pop(URLNode_t** head) {
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

bool stack_url_isempty(URLNode_t* head) {
    return head == 0;
}

int stack_url_length(URLNode_t* head) {
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

bool stack_url_contains(URLNode_t* head, char* url) {
    URLNode_t* tmp = head;
    if(tmp == NULL) {
        return false;
    }

    for(; tmp != NULL; tmp = tmp->next) {
        if(strcmp(tmp->url, url) == 0) {
            return true;
        }
    }

    return false;
}
