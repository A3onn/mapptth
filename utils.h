#ifndef UTILS_H
#define UTILS_H

#include "logger.h"
#include "stack_urls.h"
#include "trie_urls.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <pcre.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>

bool url_not_seen(char* url, struct TrieNode* urls_done, URLNode_t* urls_todo);

bool is_same_domain(char* domain_to_compare, char* domain, bool allow_subdomain);

bool is_valid_link(const char* url);

char* normalize_path(char* path, bool is_directory);

char* trim_spaces(char* str);

bool is_disallowed_path(char* path, pcre** disallowed_paths, int count_disallowed_paths);

bool is_allowed_path(char* path, pcre** allowed_paths, int count_allowed_paths);

bool is_allowed_extension(char* path, char** allowed_extensions, int count_allowed_extensions);

bool is_disallowed_extension(char* path, char** disallowed_extensions, int count_disallowed_extensions);

bool is_in_valid_domains(char* domain, char** allowed_domains, int count_allowed_domains, bool allow_subdomain);

bool is_in_disallowed_domains(char* domain, char** disallowed_domains, int count_disallowed_domains);

bool is_allowed_port(unsigned short port, unsigned short* allowed_ports, int count_allowed_short);

unsigned int get_path_depth(char* path);

char* get_base_tag_value(lxb_html_document_t* document);

unsigned short get_port_from_url(char* url);
#endif
