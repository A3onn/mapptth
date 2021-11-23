#ifndef UTILS_H
#define UTILS_H

#define RED "\033[0;31m"
#define BRIGHT_RED "\033[0;91m"
#define BLUE "\033[0;34m"
#define GREEN "\033[0;32m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define YELLOW "\033[0;33m"
#define RESET "\033[0m"

#include <stdarg.h>
#include "stack_urls.h"
#include "trie_urls.h"
#include <pcre.h>
#include <lexbor/html/html.h>

int url_not_seen(char* url, struct TrieNode* urls_done, URLNode_t* urls_todo);

int is_same_domain(char* domain_to_compare, char* domain, int allow_subdomain);

int is_valid_link(const char* url);

char* normalize_path(char* path, int is_directory);

int is_disallowed_path(char* path, pcre** disallowed_paths, int count_disallowed_paths);

int is_allowed_path(char* path, pcre** allowed_paths, int count_allowed_paths);

int is_allowed_extension(char* path, char** allowed_extensions, int count_allowed_extensions);

int is_disallowed_extension(char* path, char** disallowed_extensions, int count_disallowed_extensions);

int is_in_valid_domains(char* domain, char** allowed_domains, int count_allowed_domains, int allow_subdomain);

int is_in_disallowed_domains(char* domain, char** disallowed_domains, int count_disallowed_domains);

int is_allowed_port(unsigned short port, unsigned short* allowed_ports, int count_allowed_short);

int get_path_depth(char* path);

char* get_base_tag_value(lxb_html_document_t* document);

unsigned short get_port_from_url(char* url);

// VERBOSE
extern int _verbose;
void _verbose_print(const char* function_name, const char* format, ...);

#define LOG(...) _verbose_print(__func__, __VA_ARGS__)

#define ACTIVATE_VERBOSE() _verbose = 1;

#endif
