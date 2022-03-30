#ifndef LINKED_LIST_DOCUMENTS_H
#define LINKED_LIST_DOCUMENTS_H

#include <stdlib.h>
#include <string.h>
#include <lexbor/html/html.h>

// FIFO linked list

struct Document {
    // this struct will be filled by the fetcher thread after successfully fetched a document
    lxb_html_document_t* lexbor_document;
    char* url;
    long status_code_http;
    size_t size;
    char* content_type;
    char* redirect_location;
};

struct DocumentNode {
    struct DocumentNode* next;
    struct Document document;
};
typedef struct DocumentNode DocumentNode_t;


void stack_document_push(DocumentNode_t** head, lxb_html_document_t* lexbor_document, char* url, long status_code, size_t size, char* content_type, char* redirect_location);

struct Document* stack_document_pop(DocumentNode_t** head);

int stack_document_length(DocumentNode_t* head);

bool stack_document_isempty(DocumentNode_t* head);


#endif
