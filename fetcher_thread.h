#ifndef FETCHER_THREAD_H
#define FETCHER_THREAD_H

#include "stack_documents.h"
#include "stack_urls.h"
#include "utils.h"
#include <curl/curl.h>
#include <lexbor/html/html.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct BundleVarsThread {  // used to needed variables to the thread
    DocumentNode_t** documents;  // stack of documents to populate
    URLNode_t** urls_todo;  // stack of URLS to fetch
    URLNode_t** urls_done;  // stack of URLS fetched
    CURLSH* curl_share;
    pthread_mutex_t* mutex;

    int* isRunning;  // let know the main thread if the thread is fetching something
    int* shouldExit;  // if thread should exit; set by the main thread

    int maxRetries;
    int timeout;
    long maxFileSize;
    int resolve_ip_versions;
    int noColor;

    char* userAgent;
};

void* fetcher_thread_func(void* bundle_arg);

static size_t processContent(const char* content, size_t size, size_t nmemb, void* userp);

#endif