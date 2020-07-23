#ifndef UTILS_H
#define UTILS_H

#include "linked_list_documents.h"
#include "linked_list_urls.h"

int canBeAdded(char* url, URLNode_t* urls_done, URLNode_t* urls_todo);

int isValidDomain(char* domainToCompare, char* domain, int canBeSubDomain);

int isValidLink(const char* url);

#endif