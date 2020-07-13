#include "linked_list_documents.h"

void pushDocumentList(DocumentNode_t** head, lxb_html_document_t* document, char* url, long status_code_http, char* content_type) {
    DocumentNode_t* newNode = (DocumentNode_t*) malloc(sizeof(DocumentNode_t));
    newNode->next = *head;
    newNode->document.document = document;
    newNode->document.url = url;
    newNode->document.status_code_http = status_code_http;
    newNode->document.content_type = content_type;
    *head = newNode;
}

struct Document* popDocumentList(DocumentNode_t** head) {
    DocumentNode_t* tmp = *head;
    if(tmp == NULL) {
        return NULL;
    }
    *head = tmp->next;
    struct Document* res = (struct Document*) malloc(sizeof(struct Document));
    res->document = tmp->document.document;
    res->url = tmp->document.url;
    res->status_code_http = tmp->document.status_code_http;
    res->content_type = tmp->document.content_type;
    free(tmp);
    tmp = NULL;
    return res;
}

int getDocumentListLength(DocumentNode_t* head) {
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