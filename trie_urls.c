#include "trie_urls.h"

struct TrieNode* trie_create() {
    struct TrieNode* root = (struct TrieNode*) malloc(sizeof (struct TrieNode));

    root->children = (struct TrieNode*) malloc(sizeof (struct TrieNode) * 2);
    root->is_end_url = false;

    root->children[0].is_end_url = false;
    root->children[1].is_end_url = false;
    root->children[0].children_count = 0;
    root->children[1].children_count = 0;
    root->children[0].type = SCHEME_T;
    root->children[1].type = SCHEME_T;
    root->children[HTTP_INDEX_T].data = "http";
    root->children[HTTPS_INDEX_T].data = "https";

    root->children_count = 2;
    return root;
}

struct TrieNode* _find_child(struct TrieNode* node, char* val, char type) {
    /* Find a child node with a given value */
    for(unsigned int i = 0; i < node->children_count; i++) {
        if(strcmp(node->children[i].data, val) == 0 && node->children[i].type == type) {
             return &(node->children[i]);
        }
    }
    return NULL;
}

void trie_add(struct TrieNode* root, char* url) {
    LOG("Adding %s in trie\n", url);
    CURLU* curl_url_handler = curl_url();
    char *scheme, *host, *port, *path, *query, *fragment;
    struct TrieNode* curr = NULL;

    curl_url_set(curl_url_handler, CURLUPART_URL, url, 0);

    // get all parts of the url
    curl_url_get(curl_url_handler, CURLUPART_SCHEME, &scheme, 0);
    curl_url_get(curl_url_handler, CURLUPART_HOST, &host, 0);
    curl_url_get(curl_url_handler, CURLUPART_PORT, &port, 0);
    curl_url_get(curl_url_handler, CURLUPART_PATH, &path, 0);
    curl_url_get(curl_url_handler, CURLUPART_QUERY, &query, 0);
    curl_url_get(curl_url_handler, CURLUPART_FRAGMENT, &fragment, 0);

    // scheme
    if(strcmp(scheme, "http") == 0) {
        curr = &(root->children[HTTP_INDEX_T]);
    } else {
        curr = &(root->children[HTTPS_INDEX_T]);
    }
    // host
    struct TrieNode* tmp = _find_child(curr, host, HOST_T);
    if(tmp == NULL) {
        if(curr->children_count == 0) {
            curr->children = (struct TrieNode*) malloc(sizeof (struct TrieNode));
        } else {
            curr->children = (struct TrieNode*) reallocarray(curr->children, curr->children_count+1, sizeof (struct TrieNode));
        }
        curr->children_count++;
        curr = &(curr->children[curr->children_count-1]);

        memset(curr, 0, sizeof (struct TrieNode));
        curr->is_end_url = false;
        curr->data = (char*) malloc(sizeof (char) * (strlen(host) + 1));
        strcpy(curr->data, host);
        curr->type = HOST_T;
    } else {
            curr = tmp;
    }

    // port
    if(port == NULL) {
        if(strcmp(scheme, "https") == 0) {
            port = "443";
        } else {
            port = "80";
        }
    }
    tmp = _find_child(curr, port, PORT_T);
    if(tmp == NULL) {
        if(curr->children_count == 0) {
            curr->children = (struct TrieNode*) malloc(sizeof (struct TrieNode));
        } else {
            curr->children = (struct TrieNode*) reallocarray(curr->children, curr->children_count+1, sizeof (struct TrieNode));
        }
        curr->children_count++;
        curr = &(curr->children[curr->children_count-1]);
        memset(curr, 0, sizeof (struct TrieNode));
        curr->is_end_url = false;
        curr->data = (char*) malloc(sizeof (char) * (strlen(port) + 1));
        strcpy(curr->data, port);
        curr->type = PORT_T;
    } else {
            curr = tmp;
    }

    // path
    char* token;
    while((token = strsep(&path, "/")) != NULL) {
        if(*token == '\0') {
            continue;
        }

        tmp = _find_child(curr, token, PATH_T);
        if(tmp == NULL) {
            if(curr->children_count == 0) {
                curr->children = (struct TrieNode*) malloc(sizeof (struct TrieNode));
            } else {
                curr->children = (struct TrieNode*) reallocarray(curr->children, curr->children_count+1, sizeof (struct TrieNode));
            }
            curr->children_count++;
            curr = &(curr->children[curr->children_count-1]);
            memset(curr, 0, sizeof (struct TrieNode));
            curr->is_end_url = false;
            curr->data = (char*) malloc(sizeof (char) * (strlen(token) + 1));
            strcpy(curr->data, token);
            curr->type = PATH_T;
        } else {
            curr = tmp;
        }
    }

    // query
    if(query != NULL && *query != '\0') {
        tmp = _find_child(curr, query, QUERY_T);
        if(tmp == NULL) {
            if(curr->children_count == 0) {
                curr->children = (struct TrieNode*) malloc(sizeof (struct TrieNode));
            } else {
                curr->children = (struct TrieNode*) reallocarray(curr->children, curr->children_count+1, sizeof (struct TrieNode));
            }
            curr->children_count++;
            curr = &(curr->children[curr->children_count-1]);
            memset(curr, 0, sizeof (struct TrieNode));
            curr->is_end_url = false;
            curr->data = (char*) malloc(sizeof (char) * (strlen(query) + 1));
            strcpy(curr->data, query);
            curr->type = QUERY_T;
        } else {
            curr = tmp;
        }
    }

    // fragment
    if(fragment != NULL && *fragment != '\0') {
        tmp = _find_child(curr, fragment, FRAGMENT_T);
        if(tmp == NULL) {
            if(curr->children_count == 0) {
                curr->children = (struct TrieNode*) malloc(sizeof (struct TrieNode));
            } else {
                curr->children = (struct TrieNode*) reallocarray(curr->children, curr->children_count+1, sizeof (struct TrieNode));
            }
            curr->children_count++;
            curr = &(curr->children[curr->children_count-1]);
            memset(curr, 0, sizeof (struct TrieNode));
            curr->is_end_url = false;
            curr->data = (char*) malloc(sizeof (char) * (strlen(fragment) + 1));
            strcpy(curr->data, fragment);
            curr->type = FRAGMENT_T;
        } else {
            curr = tmp;
        }
    }
    curr->is_end_url = true;
    LOG("Finished to add %s in trie\n", url);
}

bool trie_contains(struct TrieNode* root, char* url) {
    LOG("Checking for %s in trie\n", url);
    CURLU* curl_url_handler = curl_url();
    char *scheme, *host, *port, *path, *query, *fragment;
    struct TrieNode* curr = NULL;

    curl_url_set(curl_url_handler, CURLUPART_URL, url, 0);

    curl_url_get(curl_url_handler, CURLUPART_SCHEME, &scheme, 0);

    if(strcmp(scheme, "http") == 0) {
        curr = &(root->children[HTTP_INDEX_T]);
    } else if(strcmp(scheme, "https") == 0) {
        curr = &(root->children[HTTPS_INDEX_T]);
    } else {
        return false;
    }

    curl_url_get(curl_url_handler, CURLUPART_HOST, &host, 0);
    curr = _find_child(curr, host, HOST_T);
    if(curr == NULL) {
        return false;
    }

    curl_url_get(curl_url_handler, CURLUPART_PORT, &port, 0);
    if(port == NULL) {
        if(strcmp(scheme, "https") == 0) {
            port = "443";
        } else {
            port = "80";
        }
    }
    curr = _find_child(curr, port, PORT_T);
    if(curr == NULL) {
        return false;
    }

    curl_url_get(curl_url_handler, CURLUPART_PATH, &path, 0);
    char *token;
    while((token = strsep(&path, "/")) != NULL) {
        if(*token == '\0') {
            continue;
        }
        curr = _find_child(curr, token, PATH_T);
        if(curr == NULL) {
            return false;
        }
    }

    curl_url_get(curl_url_handler, CURLUPART_QUERY, &query, 0);
    if(query != NULL) {
        curr = _find_child(curr, query, QUERY_T);
        if(curr == NULL) {
            return false;
        }
    }

    curl_url_get(curl_url_handler, CURLUPART_FRAGMENT, &fragment, 0);
    if(fragment != NULL) {
        curr = _find_child(curr, fragment, FRAGMENT_T);
        if(curr == NULL) {
            return false;
        }
    }

    if(!curr->is_end_url) {
            return false;
    }
    return true;
}

void _trie_free(struct TrieNode node) {
    for(unsigned int i = 0; i < node.children_count; i++) {
        _trie_free(node.children[i]);
    }
    if(node.type == SCHEME_T) {
        // don't free the scheme because it is not dynamicaly allocated and thus cannot be freed
        return ;
    }
    free(node.data);
    if(node.children_count != 0) {
        free(node.children);
    }
}

void trie_free(struct TrieNode* root) {
    for(unsigned int i = 0; i < root->children_count; i++) {
        _trie_free(root->children[i]);
    }
}

void _print_trie(struct TrieNode* root, int depth) {
    // Prints the nodes of the trie
    if (!root)
        return;
    struct TrieNode* tmp = root;

    for(int i = 0; i < depth; i++) {printf("\t");}
    switch(tmp->type) {
            case FRAGMENT_T:
                    printf("#%s", tmp->data);
                    break;
            case QUERY_T:
                    printf("?%s", tmp->data);
                    break;
            default:
                    printf("%s", tmp->data);
                    break;
    }
    if(tmp->is_end_url) {
        printf("\n");
    } else {
        printf(" ->\n");
    }
    for(unsigned int i = 0; i < tmp->children_count; i++) {
        _print_trie(&(tmp->children[i]), depth+1);
    }
}
void print_trie(struct TrieNode* root) {
        _print_trie(root, 0);
}
