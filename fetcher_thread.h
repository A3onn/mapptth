#ifndef FETCHER_THREAD_H
#define FETCHER_THREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
#include "linked_list_documents.h"
#include "linked_list_urls.h"

#define CHECK_LXB(X) if((X) != LXB_STATUS_OK) {fprintf(stderr,"An error occured line %i: %i\n", __LINE__, (lxb_status_t)(X));}


extern pthread_mutex_t mutexFetcher;
struct ListsThreads { // used to pass linked lists needed by the thread
    DocumentNode_t** documents; // list of documents
    URLNode_t** urls; // list of URLS
};

void* fetcher_thread_func(void* lists_arg);

static size_t processContent(const char* content, size_t size, size_t nmemb, void* userp);

#endif