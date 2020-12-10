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

int url_not_seen(char* url, URLNode_t* urls_done, URLNode_t* urls_todo);

int is_same_domain(char* domain_to_compare, char* domain, int allow_subdomain);

int is_valid_link(const char* url);

char* normalize_path(char* path, int is_directory);

int is_disallowed_path(char* path, char** disallowed_paths, int count_disallowed_paths);

int is_allowed_extension(char* path, char** allowed_extensions, int count_allowed_extensions);

int is_in_valid_domains(char* domain, char** allowed_domains, int count_allowed_domains, int allow_subdomain);

int get_path_depth(char* path);
#endif