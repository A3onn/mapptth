#include "stack_documents.h"
#include <lexbor/html/html.h>
#include <stdlib.h>
#include <string.h>

void stack_document_push(DocumentNode_t** head, lxb_html_document_t* lexbor_document, char* url, long status_code_http, size_t size, char* content_type, char* redirect_location) {
    DocumentNode_t* new_node = (DocumentNode_t*) malloc(sizeof(DocumentNode_t));
    new_node->next = *head;
    new_node->document.lexbor_document = lexbor_document;
    new_node->document.url = url;
    new_node->document.size = size;
    new_node->document.status_code_http = status_code_http;
    if(content_type != NULL) {
        new_node->document.content_type = content_type;
    } else {
        // content_type will be freed by the main thread, so it has to be in the heap
        char* default_content_type = (char*) calloc(2, sizeof (char));
        strcat(default_content_type, " ");
        new_node->document.content_type = default_content_type;
    }
    new_node->document.redirect_location = redirect_location;
    *head = new_node;
}

struct Document* stack_document_pop(DocumentNode_t** head) {
    DocumentNode_t* tmp = *head;
    if(tmp == NULL) {
        return NULL;
    }
    *head = tmp->next;
    struct Document* res = (struct Document*) malloc(sizeof(struct Document));
    res->lexbor_document = tmp->document.lexbor_document;
    res->url = tmp->document.url;
    res->status_code_http = tmp->document.status_code_http;
    res->size = tmp->document.size;
    res->content_type = tmp->document.content_type;
    res->redirect_location = tmp->document.redirect_location;
    free(tmp);
    tmp = NULL;
    return res;
}

int stack_document_length(DocumentNode_t* head) {
    DocumentNode_t* tmp = head;
    if(tmp == NULL) {
        return 0;
    }

    int res = 0;
    for(; tmp != NULL; tmp = tmp->next) {
        res++;
    }

    return res;
}

int stack_document_isempty(DocumentNode_t* head) {
    return head == NULL;
}