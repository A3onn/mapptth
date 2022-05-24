#ifndef FETCHER_THREAD_H
#define FETCHER_THREAD_H

#include "stack_documents.h"
#include "stack_urls.h"
#include "trie_urls.h"
#include "pthread.h"
#include <curl/curl.h>


struct BundleVarsThread {  // used to needed variables to the thread
    DocumentNode_t** documents;  // stack of documents to populate
    URLNode_t** urls_stack_todo;  // stack of URLS to fetch
    struct TrieNode** urls_done;  // trie keeping all urls found
    CURLSH* curl_share;
    pthread_mutex_t* mutex;
    pthread_cond_t* cv_url_added;
    pthread_cond_t* cv_fetcher_produced;

    bool* is_running;  // let know the main thread if the thread is fetching something
    bool* should_exit;  // if thread should exit; set by the main thread
    unsigned int ignore_cert_validation;

    int timeout;
    int resolve_ip_versions;
    bool no_color;

    char* user_agent;
    char* cookies;

    char* proxy_url;
    
    struct curl_slist* headers;

    // used for logging purposes
    pthread_key_t thread_key;
    unsigned int thread_number;
};

void* fetcher_thread_func(void* bundle_arg);
pthread_t* fetcher_threads;
#endif
