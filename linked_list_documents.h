#ifndef LINKED_LIST_DOCUMENTS_H
#define LINKED_LIST_DOCUMENTS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lexbor/html/html.h>

struct DocumentNode {
    struct DocumentNode* next;
    lxb_html_document_t* document;
};
typedef struct DocumentNode DocumentNode_t;

DocumentNode_t* createDocumentNode(lxb_html_document_t* document);

void pushDocumentList(DocumentNode_t** head, lxb_html_document_t* document);

lxb_html_document_t* popDocumentList(DocumentNode_t** head);

int getDocumentListLength(DocumentNode_t* head);

#endif