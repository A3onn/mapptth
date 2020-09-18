#ifndef UTILS_H
#define UTILS_H

#include "stack_documents.h"
#include "stack_urls.h"

#define RED "\033[0;31m"
#define BRIGHT_RED "\033[0;91m"
#define BLUE "\033[0;34m"
#define GREEN "\033[0;32m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

int canBeAdded(char* url, URLNode_t* urls_done, URLNode_t* urls_todo);

int isValidDomain(char* domainToCompare, char* domain, int canBeSubDomain);

int isValidLink(const char* url);

char* normalizePath(char* path);

#endif