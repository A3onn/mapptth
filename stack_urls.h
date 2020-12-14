#ifndef LINKED_LIST_URLS_H
#define LINKED_LIST_URLS_H

// FIFO linked list

struct URLNode {
    struct URLNode* next;
    char* url;
};
typedef struct URLNode URLNode_t;

void stack_url_push(URLNode_t** head, char* url);

char* stack_url_pop(URLNode_t** head);

int stack_url_length(URLNode_t* head);

int stack_url_isempty(URLNode_t* head);

int stack_url_contains(URLNode_t* head, char* url);
#endif
