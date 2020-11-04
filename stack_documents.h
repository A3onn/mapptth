#ifndef LINKED_LIST_DOCUMENTS_H
#define LINKED_LIST_DOCUMENTS_H

// FIFO

#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !CURL_AT_LEAST_VERSION(7, 62, 0)
#error "this example requires curl 7.62.0 or later"
#endif

struct Document {
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

#endif