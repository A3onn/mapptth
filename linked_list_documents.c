#include "linked_list_documents.h"

void pushDocumentList(DocumentNode_t** head, lxb_html_document_t* document) {
    DocumentNode_t* newNode = (DocumentNode_t*) malloc(sizeof (DocumentNode_t));
    newNode->next = *head;
    newNode->document = document;
    *head = newNode;
}

lxb_html_document_t* popDocumentList(DocumentNode_t** head) {
    DocumentNode_t* tmp = *head;
    if(tmp == NULL) {
        return NULL;
    }
    *head = tmp->next;
    lxb_html_document_t* res = tmp->document;
    free(tmp);
    return res;
}

DocumentNode_t* createDocumentNode(lxb_html_document_t* document) {
    DocumentNode_t* newNode = (DocumentNode_t*) malloc(sizeof (DocumentNode_t));
    newNode->document = document;
    newNode->next = NULL;
    return newNode;
}

int getDocumentListLength(DocumentNode_t* head) {
    DocumentNode_t* tmp = head;
    if(tmp->next == NULL) {
        return 0;
    }

    int res = 0;
    for(; tmp != NULL; tmp = tmp->next){res++;}

    return res;
}