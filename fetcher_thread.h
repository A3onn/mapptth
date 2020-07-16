#ifndef FETCHER_THREAD_H
#define FETCHER_THREAD_H

#include "linked_list_documents.h"
#include "linked_list_urls.h"
#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK_LXB(X) \
    if((X) != LXB_STATUS_OK) { fprintf(stderr, "An error occured line %i: %i\n", __LINE__, (lxb_status_t)(X)); }

struct BundleVarsThread {  // used to needed variables to the thread
    DocumentNode_t** documents;  // list of documents to populate
    URLNode_t** urls_todo;  // list of URLS to fetch
    URLNode_t** urls_done;  // list of URLS fetched

    pthread_mutex_t* mutex;

    int* isRunning;  // let know the main thread if the thread is fetching something

    CURLSH* curl_share;
    int maxRetries;
    int timeout;
    long maxFileSize;
};

void* fetcher_thread_func(void* bundle_arg);

static size_t processContent(const char* content, size_t size, size_t nmemb, void* userp);

#endif