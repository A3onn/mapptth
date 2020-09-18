#include "stack_documents.h"

void pushDocumentStack(DocumentNode_t** head, lxb_html_document_t* document, char* url, long status_code_http, size_t size, char* content_type, char* redirect_location) {
    DocumentNode_t* newNode = (DocumentNode_t*) malloc(sizeof(DocumentNode_t));
    newNode->next = *head;
    newNode->document.document = document;
    newNode->document.url = url;
    newNode->document.size = size;
    newNode->document.status_code_http = status_code_http;
    newNode->document.content_type = content_type;
    newNode->document.redirect_location = redirect_location;
    *head = newNode;
}

struct Document* popDocumentStack(DocumentNode_t** head) {
    DocumentNode_t* tmp = *head;
    if(tmp == NULL) {
        return NULL;
    }
    *head = tmp->next;
    struct Document* res = (struct Document*) malloc(sizeof(struct Document));
    res->document = tmp->document.document;
    res->url = tmp->document.url;
    res->status_code_http = tmp->document.status_code_http;
    res->size = tmp->document.size;
    res->content_type = tmp->document.content_type;
    res->redirect_location = tmp->document.redirect_location;
    free(tmp);
    tmp = NULL;
    return res;
}

int getDocumentStackLength(DocumentNode_t* head) {
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