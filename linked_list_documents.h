#ifndef LINKED_LIST_DOCUMENTS_H
#define LINKED_LIST_DOCUMENTS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lexbor/html/html.h>
#include <curl/curl.h>

#if !CURL_AT_LEAST_VERSION(7, 62, 0)
#error "this example requires curl 7.62.0 or later"
#endif

struct Document {
    lxb_html_document_t* document;
    CURLU* url;
};

struct DocumentNode {
    struct DocumentNode* next;
    struct Document document;
};
typedef struct DocumentNode DocumentNode_t;

int pushDocumentList(DocumentNode_t** head, lxb_html_document_t* document, char* url);

struct Document* popDocumentList(DocumentNode_t** head);

int getDocumentListLength(DocumentNode_t* head);

#endif