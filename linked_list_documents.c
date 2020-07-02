#include "linked_list_documents.h"

int pushDocumentList(DocumentNode_t** head, lxb_html_document_t* document, char* url) {
    DocumentNode_t* newNode = (DocumentNode_t*) malloc(sizeof (DocumentNode_t));
    newNode->next = *head;
    newNode->document.document = document;
    newNode->document.url = curl_url();
    if(!newNode->document.url)
        return 0;
    if(curl_url_set(newNode->document.url, CURLUPART_URL, url, 0) == 1) {
        return 0;
    }
    *head = newNode;
    return 1;
}

struct Document* popDocumentList(DocumentNode_t** head) {
    DocumentNode_t* tmp = *head;
    if(tmp == NULL) {
        return NULL;
    }
    *head = tmp->next;
    struct Document* res = (struct Document*) malloc(sizeof (struct Document));
    res->document = tmp->document.document;
    res->url = tmp->document.url;
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
    for(; tmp != NULL; tmp = tmp->next){res++;}

    return res;
}