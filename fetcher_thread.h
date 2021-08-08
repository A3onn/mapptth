#ifndef FETCHER_THREAD_H
#define FETCHER_THREAD_H

#include "stack_documents.h"
#include "stack_urls.h"
#include <curl/curl.h>


struct BundleVarsThread {  // used to needed variables to the thread
    DocumentNode_t** documents;  // stack of documents to populate
    URLNode_t** urls_stack_todo;  // stack of URLS to fetch
    URLNode_t** urls_stack_done;  // stack of URLS fetched
    CURLSH* curl_share;
    pthread_mutex_t* mutex;
    pthread_cond_t* cv_url_added;
    pthread_cond_t* cv_fetcher_produced;

    int* is_running;  // let know the main thread if the thread is fetching something
    int* should_exit;  // if thread should exit; set by the main thread
    unsigned int ignore_cert_validation;

    int timeout;
    int resolve_ip_versions;
    int no_color;

    char* user_agent;
    char* cookies;


	struct curl_slist* headers;
};

void* fetcher_thread_func(void* bundle_arg);

static size_t __fetcher_content_callback(const char* content, size_t size, size_t nmemb, void* userp);

#endif
