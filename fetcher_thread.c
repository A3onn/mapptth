#include <pthread.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <lexbor/html/html.h>
#include "fetcher_thread.h"
#include "stack_documents.h"
#include "stack_urls.h"
#include "utils.h"


struct ProcessContentBundle {
    lxb_html_document_t* lxb_document;
    size_t size;  // size of the document without the headers
    // using this variable to have the size of the document
    // even if the server doesn't send it
};

static size_t __fetcher_content_callback(const char* content, size_t size, size_t nmemb, void* userp) {
    // called when getting data
    // passing the data received to the document using chunk parsing
    struct ProcessContentBundle* doc = (struct ProcessContentBundle*) userp;
    lxb_status_t status = lxb_html_document_parse_chunk(doc->lxb_document, (lxb_char_t*) content, size * nmemb);
    if(status != LXB_STATUS_OK) {
        fprintf(stderr, "An error occured while parsing...\n");
        return 0; // indicate libcurl that we didn't parse anything, meaning an error occurred
    }
    doc->size += size * nmemb;

    return size * nmemb;
}

void* fetcher_thread_func(void* bundle_arg) {
    struct BundleVarsThread* bundle = (struct BundleVarsThread*) bundle_arg;
    DocumentNode_t** documents = bundle->documents;
    URLNode_t** urls_stack_todo = bundle->urls_stack_todo;
    URLNode_t** urls_stack_done = bundle->urls_stack_done;
    pthread_mutex_t* mutex = bundle->mutex;
    pthread_cond_t* cv_url_added = bundle->cv_url_added;
    pthread_cond_t* cv_fetcher_produced = bundle->cv_fetcher_produced;
    int* is_running = bundle->is_running;
    int* should_exit = bundle->should_exit;

    CURLcode status_c;
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTPS | CURLPROTO_HTTP);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, __fetcher_content_callback);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, bundle->timeout);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, bundle->resolve_ip_versions);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, bundle->user_agent);
    curl_easy_setopt(curl, CURLOPT_SHARE, bundle->curl_share);
    curl_easy_setopt(curl, CURLOPT_COOKIE, bundle->cookies);
    if(bundle->headers != NULL) {
        curl_easy_setopt(curl, CURLOPT_HEADEROPT, CURLHEADER_SEPARATE); // don't send headers to the proxy if using one
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, bundle->headers);
    }
    if(bundle->ignore_cert_validation) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    if(bundle->proxy_url != NULL) {
        curl_easy_setopt(curl, CURLOPT_PROXY, bundle->proxy_url);
    }

    lxb_status_t status_l;
    struct ProcessContentBundle* current_document = (struct ProcessContentBundle*) malloc(sizeof(struct ProcessContentBundle));
    char* current_url;
    char* content_type;
    char* redirect_location;
    long status_code_http;

    LOG("Ready to fetch!\n");

    while(*should_exit == 0) {
        pthread_mutex_lock(mutex);
        while(stack_url_isempty(*urls_stack_todo)) {  // no url to fetch
            *is_running = 0;  // change state
            LOG("Waiting for an URL to fetch...\n");

            // signal that this thread is waiting, the main thread checks everytime
            // if the program should exit when this signal is send and he is waiting
            pthread_cond_signal(cv_fetcher_produced);

            pthread_cond_wait(cv_url_added, mutex);
            if(*should_exit) { // main will send a broadcast when it is quitting and set should_exit to 1
                pthread_mutex_unlock(mutex);
                goto cleanup_and_exit;
            }
        }
        *is_running = 1; // indicates the main thread that this thread is fetching
        current_url = stack_url_pop(urls_stack_todo);
        stack_url_push(urls_stack_done, current_url);
        pthread_mutex_unlock(mutex);
        LOG("Got this URL to fetch: %s\n", current_url);

        current_document->lxb_document = lxb_html_document_create();
        current_document->size = 0L;
        if(current_document->lxb_document == NULL) {
            fprintf(stderr, "lxb_html_document_create failed for %s. Not doing it.\n", current_url);
            continue;
        }

        status_c = curl_easy_setopt(curl, CURLOPT_URL, current_url);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt for URL failed for %s. Not doing it. Error: %s.\n", current_url, curl_easy_strerror(status_c));
            lxb_html_document_destroy(current_document->lxb_document);
            continue;
        }
        status_c = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*) current_document);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_setopt for setting the document to write in failed for %s. Not doing it. Error: %s.\n", current_url, curl_easy_strerror(status_c));
            lxb_html_document_destroy(current_document->lxb_document);
            continue;
        }

        // fetch
        lxb_html_document_parse_chunk_begin(current_document->lxb_document);

        LOG("Fetching %s...\n", current_url);
        status_c = curl_easy_perform(curl);

        if(status_c == CURLE_OPERATION_TIMEDOUT) {
            if(current_document->size > 0) { // if it timed out because the file took too much time to send
                if(!bundle->no_color) {
                    fprintf(stderr, "%s%s : Timed out, took too long to send.%s\n", BRIGHT_RED, current_url, RESET);
                } else {
                    fprintf(stderr, "%s : Timed out, took too long to send.\n", current_url);
                }
                lxb_html_document_parse_chunk_end(current_document->lxb_document);
                lxb_html_document_destroy(current_document->lxb_document);
                continue;
            } else { // timed out without sending anything
                if(!bundle->no_color) {
                    fprintf(stderr, "%s%s : Timed out.%s\n", BRIGHT_RED, current_url, RESET);
                } else {
                    fprintf(stderr, "%s : Timed out.\n", current_url);
                }
                lxb_html_document_parse_chunk_end(current_document->lxb_document);
                lxb_html_document_destroy(current_document->lxb_document);
                continue;
            }
        } else if(status_c != CURLE_OK) {
            if(!bundle->no_color) {
                fprintf(stderr, "%s%s : %s.%s\n", BRIGHT_RED, current_url, curl_easy_strerror(status_c), RESET);
            } else {
                fprintf(stderr, "%s : %s.\n", current_url, curl_easy_strerror(status_c));
            }
            lxb_html_document_parse_chunk_end(current_document->lxb_document);
            lxb_html_document_destroy(current_document->lxb_document);
            continue;
        }

        status_l = lxb_html_document_parse_chunk_end(current_document->lxb_document);

        LOG("Getting infos about %s\n", current_url);

        status_c = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code_http);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the HTTP status code failed for %s. Setting it to 0. Error: %s.\n", current_url, curl_easy_strerror(status_c));
            status_code_http = 0L;
        }

        status_c = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the content type failed for %s. Error: %s.\n", current_url, curl_easy_strerror(status_c));
            status_code_http = 0L;
        } else if(content_type != NULL) {
            // content_type is pointing to curl's private memory
            // and it's value will change when another curl_easy_perform will occur.
            content_type = strdup(content_type);
        }

        status_c = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &redirect_location);
        if(status_c != CURLE_OK) {
            fprintf(stderr, "curl_easy_getinfo for getting the redirect URL failed for %s. Error: %s.\n", current_url, curl_easy_strerror(status_c));
        }
        if(redirect_location != NULL) {
            redirect_location = strdup(redirect_location);
        }

        pthread_mutex_lock(mutex);
        stack_document_push(documents, current_document->lxb_document, current_url, status_code_http, current_document->size, content_type, redirect_location);
        pthread_mutex_unlock(mutex);
        pthread_cond_signal(cv_fetcher_produced);
        LOG("Done with %s\n", current_url);
    }

cleanup_and_exit:
    LOG("Quitting...\n");
    curl_easy_cleanup(curl);
    free(current_document);

    return NULL;
}
