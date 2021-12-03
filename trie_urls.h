#ifndef TRIE_URL_H
#define TRIE_URL_H

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include "logger.h"

#define SCHEME_T (int) 0
#define HOST_T (int) 1
#define PORT_T (int) 2
#define PATH_T (int) 3
#define QUERY_T (int) 4
#define FRAGMENT_T (int) 5

#define HTTP_INDEX_T 0
#define HTTPS_INDEX_T 1

struct TrieNode {
    char* data;
    struct TrieNode* children;
    unsigned int children_count;
    int is_end_url;
    int type;
};

struct TrieNode* trie_create();

void trie_free(struct TrieNode* root);

int trie_contains(struct TrieNode* root, char* url);

void trie_add(struct TrieNode* root, char* url);

void print_trie(struct TrieNode* root);

struct TrieNode* _find_child(struct TrieNode* node, char* val, char type); // used for tests
#endif
